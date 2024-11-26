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
    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init error: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    // Load the font
    font = TTF_OpenFont("./fonts/PixelPowerline-9xOK.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        quit = true;
        return;
    }

    initGameObjects();
}

// Destructor
Game2::~Game2() {
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

    // Register event handlers
    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
        });

    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
        });

    // Create the player object
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(
        SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - 60, PLAYER_WIDTH, PLAYER_HEIGHT));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(0, 255, 0)); // Green player
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create aliens in a grid
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
            if (gameOver) {
                resetGame();
                gameOver = false;
            }

            handleEvents();
            receiveServerUpdates(); // Integrate server updates
            update();
            render();
            SDL_Delay(16); // ~60 FPS
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception in main loop: " << ex.what() << std::endl;
            quit = true;
        }
    }
}

// Handle events
void Game2::handleEvents() {
    static bool isSpacePressed = false;

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }

    const Uint8* keystates = SDL_GetKeyboardState(nullptr);
    auto& propertyManager = PropertyManager::getInstance();
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    if (keystates[SDL_SCANCODE_LEFT]) {
        playerVel->vx = -5;
    }
    else if (keystates[SDL_SCANCODE_RIGHT]) {
        playerVel->vx = 5;
    }
    else {
        playerVel->vx = 0;
    }

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

    pos.x = playerRect->x;
    pos.y = playerRect->y;

    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    reqSocket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    reqSocket.recv(reply);

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
            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            allPlayers[id] = pos; // Update player positions
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
    propertyManager.addProperty(projectileID, "Render", std::make_shared<RenderProperty>(255, 255, 255));
    propertyManager.addProperty(projectileID, "Velocity", std::make_shared<VelocityProperty>(0, -10));
    projectileIDs.push_back(projectileID);
}

// Update game state
void Game2::update() {
    auto& propertyManager = PropertyManager::getInstance();

    // Store IDs of objects to be removed
    std::vector<int> aliensToRemove;
    std::vector<int> projectilesToRemove;

    // Collision detection
    for (int projID : projectileIDs) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(projID, "Rect"));
        SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };

        for (int alienID : alienIDs) {
            auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
            SDL_Rect alienSDL = { alienRect->x, alienRect->y, alienRect->w, alienRect->h };

            if (SDL_HasIntersection(&projSDL, &alienSDL)) {
                // Add IDs to removal lists
                aliensToRemove.push_back(alienID);
                projectilesToRemove.push_back(projID);

                // Raise death events for the alien and projectile
                EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(alienID, &gameTimeline));
                EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(projID, &gameTimeline));
            }
        }
    }

    // Remove aliens and projectiles that were hit
    for (int alienID : aliensToRemove) {
        alienIDs.erase(std::remove(alienIDs.begin(), alienIDs.end(), alienID), alienIDs.end());
        propertyManager.destroyObject(alienID);
    }

    for (int projID : projectilesToRemove) {
        projectileIDs.erase(std::remove(projectileIDs.begin(), projectileIDs.end(), projID), projectileIDs.end());
        propertyManager.destroyObject(projID);
    }

    // Update other game objects
    updateGameObjects();
}


// Update game objects
void Game2::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    playerRect->x += playerVel->vx;

    // Ensure the player stays within screen bounds
    if (playerRect->x < 0) playerRect->x = 0;
    if (playerRect->x + PLAYER_WIDTH > SCREEN_WIDTH) playerRect->x = SCREEN_WIDTH - PLAYER_WIDTH;

    // Update player projectiles
    for (auto it = projectileIDs.begin(); it != projectileIDs.end();) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(*it, "Rect"));
        projRect->y -= 10; // Move projectile upward

        // Remove projectile if it goes off-screen
        if (projRect->y + PROJECTILE_HEIGHT < 0) {
            propertyManager.destroyObject(*it);
            it = projectileIDs.erase(it);
        }
        else {
            ++it;
        }
    }

    // Alien movement logic
    static int alienMoveTimer = 0;
    static int alienDirection = 1; // 1 for right, -1 for left

    ++alienMoveTimer;
    float tic = gameTimeline.getTic(); // Get current tic rate

    if (alienMoveTimer >= static_cast<int>(30 / tic)) { // Adjust timer by timeline tic rate
        alienMoveTimer = 0;

        bool changeDirection = false;
        for (int alienID : alienIDs) {
            auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
            alienRect->x += alienDirection * 10;

            if (alienRect->x < 0 || alienRect->x + ALIEN_WIDTH > SCREEN_WIDTH) {
                changeDirection = true;
            }
        }

        if (changeDirection) {
            alienDirection *= -1;
            for (int alienID : alienIDs) {
                auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
                alienRect->y += 20;
            }
        }
    }

    // Alien shooting logic
    static int alienShootTimer = 0;
    ++alienShootTimer;

    if (alienShootTimer >= static_cast<int>(100 / tic)) { // Adjust shooting timer by timeline tic rate
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
        }
    }

    // Update alien projectiles
    for (auto it = alienProjectileIDs.begin(); it != alienProjectileIDs.end();) {
        auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(*it, "Rect"));
        projRect->y += 5; // Move projectile downward

        SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
        SDL_Rect playerSDL = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

        // Check for collision with player
        if (SDL_HasIntersection(&projSDL, &playerSDL)) {
            // Destroy player and projectile
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(playerID, &gameTimeline));
            propertyManager.destroyObject(playerID);
            propertyManager.destroyObject(*it);
            it = alienProjectileIDs.erase(it);

            // Mark the game as over
            std::cout << "Player has been destroyed! Resetting game..." << std::endl;
            gameOver = true; // Set the game over flag
        }
        else if (projRect->y > SCREEN_HEIGHT) { // Remove projectile if off-screen
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

        // Increment the tic rate for the next level
        gameTimeline.changeTic(gameTimeline.getTic() + 0.5f);

        // Reset the game state
        resetGame();
    }
}


// Render all game objects
void Game2::render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

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

    renderLevelText(); // Render the level text

    SDL_RenderPresent(renderer);
}


void Game2::renderAlienProjectile(int alienProjID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienProjID, "Rect"));
    SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow projectiles
    SDL_RenderFillRect(renderer, &projSDL);
}


// Render player
void Game2::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();

    if (!propertyManager.hasObject(playerID)) {
        // If the player object doesn’t exist, return silently
        return;
    }

    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    SDL_Rect playerSDL = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &playerSDL);
}



// Render alien
void Game2::renderAlien(int alienID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto alienRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(alienID, "Rect"));
    SDL_Rect alienSDL = { alienRect->x, alienRect->y, alienRect->w, alienRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &alienSDL);
}

// Render projectile
void Game2::renderProjectile(int projectileID) {
    auto& propertyManager = PropertyManager::getInstance();
    auto projRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(projectileID, "Rect"));
    SDL_Rect projSDL = { projRect->x, projRect->y, projRect->w, projRect->h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &projSDL);
}

// Handle spawn events
void Game2::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;
}

// Handle death events
void Game2::handleDeath(int objectID) {
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    if (objectID == playerID) {
        std::cout << "Player has been hit. Preparing to reset the game..." << std::endl;

        // Mark the game as over
        gameOver = true;

        // Avoid accessing invalid state
        return;
    }

    // Destroy the object as usual
    PropertyManager::getInstance().destroyObject(objectID);
}

void Game2::resetGame() {
    auto& propertyManager = PropertyManager::getInstance();

    // Destroy all existing objects
    for (int projID : projectileIDs) {
        propertyManager.destroyObject(projID);
    }
    projectileIDs.clear();

    for (int alienProjID : alienProjectileIDs) {
        propertyManager.destroyObject(alienProjID);
    }
    alienProjectileIDs.clear();

    for (int alienID : alienIDs) {
        propertyManager.destroyObject(alienID);
    }
    alienIDs.clear();

    if (propertyManager.hasObject(playerID)) {
        propertyManager.destroyObject(playerID);
    }

    // Check if reset is due to player death
    if (gameOver) {
        // Reset level and timeline to default values
        level = 1;
        gameTimeline.changeTic(1.0f); // Reset tic to normal speed
        std::cout << "Player hit! Game reset to level 1 with default speed." << std::endl;

        // Ensure `gameOver` is reset before reinitialization
        gameOver = false;
    }
    else {
        // Increment the level if all aliens are destroyed
        ++level;
        float newTic = 1.0f + (level - 1) * 0.3f; // Increase speed by 30% per level
        gameTimeline.changeTic(newTic);
        std::cout << "All aliens destroyed! Moving to level " << level
            << " with tic rate: " << newTic << "." << std::endl;
    }

    // Reinitialize the game objects
    initGameObjects();

    // Debug message to confirm reset
    std::cout << "Game objects reinitialized for level " << level << "." << std::endl;

    // Ensure the `quit` flag is not triggered during reset
    quit = false;
}

void Game2::renderLevelText() {
    // Free existing texture if it exists
    if (levelTexture) {
        SDL_DestroyTexture(levelTexture);
        levelTexture = nullptr;
    }

    // Create a surface with the level text
    std::string levelText = "Level: " + std::to_string(level);
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, levelText.c_str(), white);
    if (!textSurface) {
        std::cerr << "TTF_RenderText_Solid error: " << TTF_GetError() << std::endl;
        return;
    }

    // Create a texture from the surface
    levelTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    if (!levelTexture) {
        std::cerr << "SDL_CreateTextureFromSurface error: " << SDL_GetError() << std::endl;
        return;
    }

    // Set the position and size of the text
    levelRect.x = 10; // Top-left corner
    levelRect.y = 10;
    SDL_QueryTexture(levelTexture, nullptr, nullptr, &levelRect.w, &levelRect.h);

    // Render the text
    SDL_RenderCopy(renderer, levelTexture, nullptr, &levelRect);
}

