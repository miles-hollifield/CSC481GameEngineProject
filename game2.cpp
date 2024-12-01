#include "game2.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include <iostream>
#include <cstring>
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"
#include "InputEvent.h"
#include "CollisionEvent.h"

// Constructor
Game2::Game2(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false),
    gameTimeline(nullptr, 1.0f), font(nullptr), levelTexture(nullptr), clientId(-1) {
    // Initialize SDL_ttf for text rendering
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    // Load the font for rendering text
    font = TTF_OpenFont("./fonts/PixelPowerline-9xOK.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    // Initialize game objects like the player and aliens
    initGameObjects();
}

// Destructor
Game2::~Game2() {
    // Clean up font and textures
    if (font) {
        TTF_CloseFont(font);
    }
    if (levelTexture) {
        SDL_DestroyTexture(levelTexture);
    }
    TTF_Quit();
}

// Initialize game objects
void Game2::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();
    auto& eventManager = EventManager::getInstance();

    // Register event handlers for SPAWN and DEATH events
    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
        });

    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
        });

    // Create the player object and initialize its properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(
        SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - 60, PLAYER_WIDTH, PLAYER_HEIGHT));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(0, 255, 0)); // Green player
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create aliens in a grid layout
    int numColumns = 10;
    int numRows = 5;
    int totalAlienWidth = numColumns * (ALIEN_WIDTH + 10) - 10; // Total width with spacing
    int startX = (SCREEN_WIDTH - totalAlienWidth) / 2;
    int startY = 50;

    for (int i = 0; i < numRows; ++i) {
        for (int j = 0; j < numColumns; ++j) {
            int alienID = propertyManager.createObject();
            propertyManager.addProperty(alienID, "Rect", std::make_shared<RectProperty>(
                startX + j * (ALIEN_WIDTH + 10), startY + i * (ALIEN_HEIGHT + 10), ALIEN_WIDTH, ALIEN_HEIGHT));
            propertyManager.addProperty(alienID, "Render", std::make_shared<RenderProperty>(255, 0, 0)); // Red aliens
            alienIDs.push_back(alienID);
        }
    }
}

// Main game loop
void Game2::run() {
    while (!quit) {
        try {
            // If the game is over, reset the game state
            if (gameOver) {
                resetGame();
                gameOver = false;
            }

            // Process game events and updates
            handleEvents();
			EventManager::getInstance().dispatchEvents();
            receiveServerUpdates(); // Integrate server updates
            update();
            render();

            // Limit frame rate to ~60 FPS
            SDL_Delay(16);
        } catch (const std::exception& ex) {
            std::cerr << "Exception in main loop: " << ex.what() << std::endl;
            quit = true;
        }
    }
}

// Handle events
void Game2::handleEvents() {
    static bool isSpacePressed = false;

    // Poll SDL events, such as quit or keyboard input
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }

    const Uint8* keystates = SDL_GetKeyboardState(nullptr);
    auto& propertyManager = PropertyManager::getInstance();
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    // Handle player movement using arrow keys
    if (keystates[SDL_SCANCODE_LEFT]) {
        playerVel->vx = -5;
    }
    else if (keystates[SDL_SCANCODE_RIGHT]) {
        playerVel->vx = 5;
    }
    else {
        playerVel->vx = 0;
    }

    // Fire a projectile when the space bar is pressed
    if (keystates[SDL_SCANCODE_SPACE] && !isSpacePressed) {
        fireProjectile();
        isSpacePressed = true;
    }
    else if (!keystates[SDL_SCANCODE_SPACE]) {
        isSpacePressed = false;
    }

    sendPlayerUpdate(); // Send player position to server
}

// Send player position to the server
void Game2::sendPlayerUpdate() {
    zmq::message_t request(sizeof(clientId) + sizeof(PlayerPosition));

    PlayerPosition pos;
    auto& propertyManager = PropertyManager::getInstance();
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Populate player position data
    pos.x = playerRect->x;
    pos.y = playerRect->y;

    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    // Send the player's position to the server
    reqSocket.send(request, zmq::send_flags::none);

    // Receive the server's response
    zmq::message_t reply;
    reqSocket.recv(reply);

    // Assign a client ID if one is not already assigned
    if (clientId == -1) {
        memcpy(&clientId, reply.data(), sizeof(clientId));
        std::cout << "Assigned client ID: " << clientId << std::endl;
    }
}

// Receive updates from the server
void Game2::receiveServerUpdates() {
    zmq::message_t update;
    if (subSocket.recv(update, zmq::recv_flags::dontwait)) {
        char* buffer = static_cast<char*>(update.data());
        while (buffer < static_cast<char*>(update.data()) + update.size()) {
            int id;
            PlayerPosition pos;

            // Parse player position updates from the server
            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            // Update the positions of all players
            allPlayers[id] = pos;
        }
    }
}

// Fire a projectile
void Game2::fireProjectile() {
    auto& propertyManager = PropertyManager::getInstance();
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    int projectileID = propertyManager.createObject();
    propertyManager.addProperty(projectileID, "Rect", std::make_shared<RectProperty>(
        playerRect->x + PLAYER_WIDTH / 2 - PROJECTILE_WIDTH / 2,
        playerRect->y,
        PROJECTILE_WIDTH,
        PROJECTILE_HEIGHT));
    propertyManager.addProperty(projectileID, "Render", std::make_shared<RenderProperty>(255, 255, 255)); // White projectile
    propertyManager.addProperty(projectileID, "Velocity", std::make_shared<VelocityProperty>(0, -10)); // Moves upward
    projectileIDs.push_back(projectileID);

    // Raise a SpawnEvent for the projectile
    EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(projectileID, &gameTimeline));
}


// Update game state
void Game2::update() {
    auto& propertyManager = PropertyManager::getInstance();

    // Store IDs of objects to be removed after collision detection
    std::vector<int> aliensToRemove;
    std::vector<int> projectilesToRemove;

    // Check for collisions between projectiles and aliens
    for (int projID : projectileIDs) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(projID, "Rect"));
        SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };

        for (int alienID : alienIDs) {
            auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
            SDL_Rect alienSDL = { alienRect->x, alienRect->y, alienRect->w, alienRect->h };

            if (SDL_HasIntersection(&projSDL, &alienSDL)) {
                // Raise death events for the destroyed objects
                EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(alienID, &gameTimeline));
                EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(projID, &gameTimeline));

                // Add IDs to removal lists for both aliens and projectiles
                aliensToRemove.push_back(alienID);
                projectilesToRemove.push_back(projID);
            }
        }
    }

    // Remove collided aliens and projectiles
    for (int alienID : aliensToRemove) {
        alienIDs.erase(std::remove(alienIDs.begin(), alienIDs.end(), alienID), alienIDs.end());
        propertyManager.destroyObject(alienID);
    }

    for (int projID : projectilesToRemove) {
        projectileIDs.erase(std::remove(projectileIDs.begin(), projectileIDs.end(), projID), projectileIDs.end());
        propertyManager.destroyObject(projID);
    }

    // Update all remaining game objects
    updateGameObjects();
}


// Update game objects
void Game2::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update the player's position based on velocity
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    playerRect->x += playerVel->vx;

    // Ensure the player remains within screen bounds
    if (playerRect->x < 0) playerRect->x = 0;
    if (playerRect->x + PLAYER_WIDTH > SCREEN_WIDTH) playerRect->x = SCREEN_WIDTH - PLAYER_WIDTH;

    // Update positions of player projectiles
    for (auto it = projectileIDs.begin(); it != projectileIDs.end();) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(*it, "Rect"));
        projRect->y -= 10; // Move projectile upward

        // Remove projectile if it goes off-screen
        if (projRect->y + PROJECTILE_HEIGHT < 0) {
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(*it, &gameTimeline));
            propertyManager.destroyObject(*it);
            it = projectileIDs.erase(it);
        }
        else {
            ++it;
        }
    }

    // Handle alien movement and shooting logic
    static int alienMoveTimer = 0;
    static int alienDirection = 1; // 1 for moving right, -1 for moving left

    ++alienMoveTimer;
    float tic = gameTimeline.getTic(); // Get the current tic rate

    if (alienMoveTimer >= static_cast<int>(30 / tic)) { // Adjust movement rate by tic
        alienMoveTimer = 0;
        bool changeDirection = false;

        for (int alienID : alienIDs) {
            auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
            alienRect->x += alienDirection * 10;

            // Check if aliens hit the screen edges
            if (alienRect->x < 0 || alienRect->x + ALIEN_WIDTH > SCREEN_WIDTH) {
                changeDirection = true;
            }
        }

        // Reverse direction and move aliens downward if they hit the screen edge
        if (changeDirection) {
            alienDirection *= -1;
            for (int alienID : alienIDs) {
                auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
                alienRect->y += 20;
            }
        }
    }

    // Handle alien shooting logic
    static int alienShootTimer = 0;
    ++alienShootTimer;

    if (alienShootTimer >= static_cast<int>(100 / tic)) { // Adjust shooting rate by tic
        alienShootTimer = 0;

        if (!alienIDs.empty()) {
            int randomIndex = rand() % alienIDs.size();
            int shootingAlienID = alienIDs[randomIndex];
            auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(shootingAlienID, "Rect"));

            // Create a projectile from the alien's position
            int alienProjID = propertyManager.createObject();
            propertyManager.addProperty(alienProjID, "Rect", std::make_shared<RectProperty>(
                alienRect->x + ALIEN_WIDTH / 2 - PROJECTILE_WIDTH / 2,
                alienRect->y + ALIEN_HEIGHT,
                PROJECTILE_WIDTH,
                PROJECTILE_HEIGHT));
            propertyManager.addProperty(alienProjID, "Render", std::make_shared<RenderProperty>(255, 255, 0)); // Yellow projectile
            propertyManager.addProperty(alienProjID, "Velocity", std::make_shared<VelocityProperty>(0, 5));   // Move downward
            alienProjectileIDs.push_back(alienProjID);

            // Raise a SpawnEvent for the new alien projectile
            EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(alienProjID, &gameTimeline));
        }
    }

    // Update alien projectiles
    for (auto it = alienProjectileIDs.begin(); it != alienProjectileIDs.end();) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(*it, "Rect"));
        projRect->y += 5; // Move projectile downward

        SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
        SDL_Rect playerSDL = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

        // Check for collision with the player
        if (SDL_HasIntersection(&projSDL, &playerSDL)) {
            // Raise DeathEvent for both the player and the projectile
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(playerID, &gameTimeline));
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(*it, &gameTimeline));

            propertyManager.destroyObject(playerID);
            propertyManager.destroyObject(*it);
            it = alienProjectileIDs.erase(it);

            // End the game as the player has been hit
            std::cout << "Player has been destroyed! Resetting game..." << std::endl;
            gameOver = true;
        }
        else if (projRect->y > SCREEN_HEIGHT) { // Remove off-screen projectiles
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(*it, &gameTimeline));
            propertyManager.destroyObject(*it);
            it = alienProjectileIDs.erase(it);
        }
        else {
            ++it;
        }
    }

    // Check if all aliens are destroyed
    if (alienIDs.empty()) {
        std::cout << "All aliens destroyed! Moving to the next level..." << std::endl;

        // Raise SpawnEvent for the next level
        EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(playerID, &gameTimeline));

        // Increase tic rate and reset the game for the next level
        gameTimeline.changeTic(gameTimeline.getTic() + 0.5f);
        resetGame();
    }
}

// Render all game objects
void Game2::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    // Render the player, aliens, and projectiles
    renderPlayer(playerID);
    for (int alienID : alienIDs) {
        renderAlien(alienID);
    }
    for (int projID : projectileIDs) {
        renderProjectile(projID);
    }
    for (int alienProjID : alienProjectileIDs) {
        renderAlienProjectile(alienProjID);
    }

    renderLevelText(); // Render level and speed information

    SDL_RenderPresent(renderer);
}

// Render an alien projectile
void Game2::renderAlienProjectile(int alienProjID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienProjID, "Rect"));
    SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow for alien projectiles
    SDL_RenderFillRect(renderer, &projSDL);
}

// Render the player object
void Game2::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();

    // Check if the player object exists
    if (!propertyManager.hasObject(playerID)) {
        // Return silently if the player doesn't exist
        return;
    }

    // Retrieve and render the player's rectangle
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    SDL_Rect playerSDL = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for the player
    SDL_RenderFillRect(renderer, &playerSDL);
}

// Render an alien object
void Game2::renderAlien(int alienID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
    SDL_Rect alienSDL = { alienRect->x, alienRect->y, alienRect->w, alienRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red for aliens
    SDL_RenderFillRect(renderer, &alienSDL);
}

// Render a projectile object
void Game2::renderProjectile(int projectileID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(projectileID, "Rect"));
    SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White for projectiles
    SDL_RenderFillRect(renderer, &projSDL);
}

// Handle a spawn event
void Game2::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;

    auto& propertyManager = PropertyManager::getInstance();

    // Find a suitable spawn position (for example, reset the player or an alien)
    SDL_Point spawnPosition = { SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100 }; // Default spawn position
    if (objectID == playerID) {
        // If it's the player, reset to the default spawn position
        auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));
        playerRect->x = spawnPosition.x;
        playerRect->y = spawnPosition.y;
        std::cout << "Player respawned at (" << spawnPosition.x << ", " << spawnPosition.y << ")" << std::endl;
    }
    else if (std::find(alienIDs.begin(), alienIDs.end(), objectID) != alienIDs.end()) {
        // If it's an alien, respawn it in a random row
        int column = rand() % 10; // Assuming 10 columns
        auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));
        alienRect->x = column * (ALIEN_WIDTH + 10) + 50;
        alienRect->y = 50; // Reset aliens to the top row
        std::cout << "Alien respawned at (" << alienRect->x << ", " << alienRect->y << ")" << std::endl;
    }
}

// Handle a death event
void Game2::handleDeath(int objectID) {
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    auto& propertyManager = PropertyManager::getInstance();

    if (objectID == playerID) {
        // If the player is destroyed, reset the game state
        std::cout << "Player destroyed. Resetting the game..." << std::endl;
        gameOver = true;
    }
    else if (std::find(alienIDs.begin(), alienIDs.end(), objectID) != alienIDs.end()) {
        // Remove the alien from the game
        alienIDs.erase(std::remove(alienIDs.begin(), alienIDs.end(), objectID), alienIDs.end());
        propertyManager.destroyObject(objectID);
        std::cout << "Alien destroyed. Remaining aliens: " << alienIDs.size() << std::endl;

        // Check if all aliens are destroyed
        if (alienIDs.empty()) {
            std::cout << "All aliens destroyed. Advancing to the next level..." << std::endl;
            resetGame();
        }
    }
    else if (std::find(projectileIDs.begin(), projectileIDs.end(), objectID) != projectileIDs.end()) {
        // Remove the projectile
        projectileIDs.erase(std::remove(projectileIDs.begin(), projectileIDs.end(), objectID), projectileIDs.end());
        propertyManager.destroyObject(objectID);
        std::cout << "Projectile destroyed." << std::endl;
    }
}

// Reset the game state
void Game2::resetGame() {
    auto& propertyManager = PropertyManager::getInstance();

    // Destroy all projectiles
    for (int projID : projectileIDs) {
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(projID, &gameTimeline));
        propertyManager.destroyObject(projID);
    }
    projectileIDs.clear();

    // Destroy all alien projectiles
    for (int alienProjID : alienProjectileIDs) {
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(alienProjID, &gameTimeline));
        propertyManager.destroyObject(alienProjID);
    }
    alienProjectileIDs.clear();

    // Destroy all aliens
    for (int alienID : alienIDs) {
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(alienID, &gameTimeline));
        propertyManager.destroyObject(alienID);
    }
    alienIDs.clear();

    // Destroy the player object if it exists
    if (propertyManager.hasObject(playerID)) {
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(playerID, &gameTimeline));
        propertyManager.destroyObject(playerID);
    }

    if (gameOver) {
        // Reset to the first level with default speed
        level = 1;
        gameTimeline.changeTic(1.0f);
        std::cout << "Player hit! Game reset to level 1 with default speed." << std::endl;
        gameOver = false;
    }
    else {
        // Increment the level and increase speed
        ++level;
        float newTic = 1.0f + (level - 1) * 0.5f; // Increase speed by 50% per level
        gameTimeline.changeTic(newTic);
        std::cout << "All aliens destroyed! Moving to level " << level
            << " with tic rate: " << newTic << "." << std::endl;
    }

    // Reinitialize all game objects for the new level
    initGameObjects();
}


// Render the level and speed text
void Game2::renderLevelText() {
    // Destroy existing textures to avoid memory leaks
    if (levelTexture) {
        SDL_DestroyTexture(levelTexture);
        levelTexture = nullptr;
    }
    if (speedTexture) {
        SDL_DestroyTexture(speedTexture);
        speedTexture = nullptr;
    }

    // Prepare the level text
    std::stringstream levelStream;
    levelStream << "Level: " << level;
    std::string levelStr = levelStream.str();

    SDL_Color white = { 255, 255, 255, 255 }; // White text color
    SDL_Surface* levelSurface = TTF_RenderText_Solid(font, levelStr.c_str(), white);
    if (!levelSurface) {
        std::cerr << "TTF_RenderText_Solid error: " << TTF_GetError() << std::endl;
        return;
    }

    levelTexture = SDL_CreateTextureFromSurface(renderer, levelSurface);
    SDL_FreeSurface(levelSurface);
    if (!levelTexture) {
        std::cerr << "SDL_CreateTextureFromSurface error: " << SDL_GetError() << std::endl;
        return;
    }

    // Render the level text at the top-left corner
    levelRect.x = 10;
    levelRect.y = 10;
    SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);
    SDL_RenderCopy(renderer, levelTexture, nullptr, &levelRect);

    // Prepare the speed text
    std::stringstream speedStream;
    speedStream << "Speed: " << std::fixed << std::setprecision(2) << gameTimeline.getTic();
    std::string speedStr = speedStream.str();

    SDL_Surface* speedSurface = TTF_RenderText_Solid(font, speedStr.c_str(), white);
    if (!speedSurface) {
        std::cerr << "TTF_RenderText_Solid error: " << TTF_GetError() << std::endl;
        return;
    }

    speedTexture = SDL_CreateTextureFromSurface(renderer, speedSurface);
    SDL_FreeSurface(speedSurface);
    if (!speedTexture) {
        std::cerr << "SDL_CreateTextureFromSurface error: " << SDL_GetError() << std::endl;
        return;
    }

    // Render the speed text next to the level text
    speedRect.x = levelRect.x + levelRect.w + 20; // Offset from the level text
    speedRect.y = levelRect.y;
    SDL_QueryTexture(speedTexture, nullptr, nullptr, &speedRect.w, &speedRect.h);
    SDL_RenderCopy(renderer, speedTexture, nullptr, &speedRect);
}
