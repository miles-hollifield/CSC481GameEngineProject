#include "game.h"
#include "PropertyManager.h" // For property-based model
#include "ThreadManager.h"
#include <iostream>
#include <cstring>

// Constructor for the Game class
Game::Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), quit(false), clientId(-1), cameraX(0), cameraY(0)
{
    // Initialize game objects, such as players, platforms, etc.
    initGameObjects();
}

// Destructor
Game::~Game() {}

// Initialize game objects (characters, platforms, etc.)
void Game::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Create player object and set its properties (position, render color, physics, etc.)
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(100, 400, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(255, 0, 0)); // Red color for player
    propertyManager.addProperty(playerID, "Physics", std::make_shared<PhysicsProperty>(10.0f));  // Gravity
    propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true)); // Enable collision
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0)); // Initial velocity

    // Create static platforms with random colors
    platformID = propertyManager.createObject();
    propertyManager.addProperty(platformID, "Rect", std::make_shared<RectProperty>(50, 700, 200, 50)); // (50, 500, 200, 50)
    propertyManager.addProperty(platformID, "Render", std::make_shared<RenderProperty>(135, 206, 235)); // Light blue color
    propertyManager.addProperty(platformID, "Collision", std::make_shared<CollisionProperty>(true));

    platformID2 = propertyManager.createObject();
    propertyManager.addProperty(platformID2, "Rect", std::make_shared<RectProperty>(250, 600, 200, 50)); // (250, 600, 200, 50)
    propertyManager.addProperty(platformID2, "Render", std::make_shared<RenderProperty>(34, 139, 34)); // Forest green color
    propertyManager.addProperty(platformID2, "Collision", std::make_shared<CollisionProperty>(true));

    platformID3 = propertyManager.createObject();
    propertyManager.addProperty(platformID3, "Rect", std::make_shared<RectProperty>(450, 500, 900, 50)); // (450, 700, 900, 50)
    propertyManager.addProperty(platformID3, "Render", std::make_shared<RenderProperty>(255, 69, 0)); // Orange red color
    propertyManager.addProperty(platformID3, "Collision", std::make_shared<CollisionProperty>(true));

    // Create a horizontally moving platform with a random color
    movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Rect", std::make_shared<RectProperty>(150, 900, 200, 50));
    propertyManager.addProperty(movingPlatformID, "Render", std::make_shared<RenderProperty>(75, 0, 130)); // Indigo color
    propertyManager.addProperty(movingPlatformID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally
    std::cout << "Moving Platform 1 Velocity initialized" << std::endl;

    // Create a vertically moving platform with a random color
    movingPlatformID2 = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID2, "Rect", std::make_shared<RectProperty>(1400, 200, 200, 50));  // Different position
    propertyManager.addProperty(movingPlatformID2, "Render", std::make_shared<RenderProperty>(255, 215, 0));  // Gold color
    propertyManager.addProperty(movingPlatformID2, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID2, "Velocity", std::make_shared<VelocityProperty>(0, 2));  // Moving vertically
    std::cout << "Moving Platform 2 Velocity initialized" << std::endl;

    // Create a spawn point for the player
    spawnPointID = propertyManager.createObject();
    propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(100, 400, 50, 50));

    // Create a death zone at the bottom of the screen
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Rect", std::make_shared<RectProperty>(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50));  // Near the bottom
    propertyManager.addProperty(deathZoneID, "Collision", std::make_shared<CollisionProperty>(true));  // Enable collision for the death zone

    // Create screen boundaries (left and right)
    rightBoundaryID = propertyManager.createObject();
    propertyManager.addProperty(rightBoundaryID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH - 50, 0, 50, SCREEN_HEIGHT));
    propertyManager.addProperty(rightBoundaryID, "Collision", std::make_shared<CollisionProperty>(true));
    rightScrollCount = 0;

    leftBoundaryID = propertyManager.createObject();
    propertyManager.addProperty(leftBoundaryID, "Rect", std::make_shared<RectProperty>(0, 0, 50, SCREEN_HEIGHT));
    propertyManager.addProperty(leftBoundaryID, "Collision", std::make_shared<CollisionProperty>(true));
    leftScrollCount = 0;
}

// Main game loop
void Game::run() {
    // Main game loop that handles events, updates, and rendering
    while (!quit) {
        handleEvents();  // Handle input and events

        receivePlayerPositions();  // Receive other players' positions from the server

        update();  // Update the game state (e.g., player movement, collision detection)

        updateGameObjects();  // Update the position of game objects (players, platforms, etc.)

        render();  // Render the game objects to the screen

        // Cap frame rate to around 60 FPS
        SDL_Delay(16);
    }
}

// Handle events, including input
void Game::handleEvents() {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;  // Exit the game if the quit event is detected
        }

        // Handle input for the local player
        auto& propertyManager = PropertyManager::getInstance();
        std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
        std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

        // Handle keyboard input for player movement
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_LEFT]) {
            playerVel->vx = -5;  // Move left
        }
        else if (keystates[SDL_SCANCODE_RIGHT]) {
            playerVel->vx = 5;  // Move right
        }
        else {
            playerVel->vx = 0;  // Stop moving horizontally
        }

        if (keystates[SDL_SCANCODE_UP] && (playerVel->vy == 1)) {
            playerVel->vy = -15.1;  // Jump
        }

        // Send the player's movement update to the server
        sendMovementUpdate();
    }

    // Update camera to follow the player
    updateCamera();
}

// Update the camera to follow the player's movement
void Game::updateCamera() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Center the camera on the player
    cameraX = playerRect->x - (SCREEN_WIDTH / 2 - playerRect->w / 2);
    cameraY = playerRect->y - (SCREEN_HEIGHT / 2 - playerRect->h / 2);

    // Prevent the camera from going out of bounds
    if (cameraX < 0) cameraX = 0;
    if (cameraY < 0) cameraY = 0;
}

// Send the player's movement data to the server
void Game::sendMovementUpdate() {
    zmq::message_t request(sizeof(clientId) + sizeof(PlayerPosition));

    // Package the player ID and position
    PlayerPosition pos;
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    pos.x = playerRect->x;
    pos.y = playerRect->y;

    // Copy the client ID and position into the request
    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    // Send the request to the server
    reqSocket.send(request, zmq::send_flags::none);

    // Receive acknowledgment from the server
    zmq::message_t reply;
    reqSocket.recv(reply);

    // If the player ID is not assigned, get the assigned ID from the server
    if (clientId == -1) {
        memcpy(&clientId, reply.data(), sizeof(clientId));
        std::cout << "Received assigned playerId: " << clientId << std::endl;
    }
}

// Receive player positions from the server and update the game state
void Game::receivePlayerPositions() {
    zmq::message_t update;
    zmq::recv_result_t recvResult = subSocket.recv(update, zmq::recv_flags::dontwait);

    if (recvResult) {
        allPlayers.clear();
        char* buffer = static_cast<char*>(update.data());

        // Deserialize player positions from the received data
        while (buffer < static_cast<char*>(update.data()) + update.size() - sizeof(PlayerPosition)) {
            int id;
            PlayerPosition pos;

            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            allPlayers[id] = pos;  // Update the positions of all players
        }
    }
}

// Handle player collisions with platforms
void Game::handleCollision(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    std::shared_ptr<PhysicsProperty> playerPhysics = std::static_pointer_cast<PhysicsProperty>(propertyManager.getProperty(playerID, "Physics"));
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };  // Player's rectangle for collision detection

    std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    SDL_Rect platRect = { platformRect->x, platformRect->y, platformRect->w, platformRect->h };  // Platform's rectangle for collision detection

    // Check for intersection between player and platform
    if (SDL_HasIntersection(&playRect, &platRect)) {
        if (playerRect->y + playerRect->h / 2 < platformRect->y) { // Player is above the platform
            playerVel->vy = 0.0f;  // Stop downward velocity
            playerRect->y = platformRect->y - playerRect->h;  // Position the player on top of the platform
        }
        // Additional checks for other collision sides
        if (playerRect->y + playerRect->h / 2 > platformRect->y + platformRect->h) { // Player is below the platform
            playerVel->vy = playerPhysics->gravity;  // Set downward velocity to simulate falling
            playerRect->y = platformRect->y + platformRect->h;  // Position the player below the platform
        }
        if (playerRect->x + playerRect->w / 2 < platformRect->x) { // Player is to the left of the platform
            playerRect->x = platformRect->x - playerRect->w;  // Position the player to the left
        }
        if (playerRect->x + playerRect->w / 2 > platformRect->x + platformRect->w) { // Player is to the right of the platform
            playerRect->x = platformRect->x + platformRect->w;  // Position the player to the right
        }
    }
}

// Check for collisions between the player and platforms/boundaries
void Game::checkCollisions() {
    auto& propertyManager = PropertyManager::getInstance();
    const auto& allProperties = propertyManager.getAllProperties();

    // Loop through all objects and check if they have a "Collision" property
    for (const auto& objectPair : allProperties) {
        int objectID = objectPair.first;

        // Check if the object has a "Collision" property and is not the player
        if (objectPair.second.count("Collision") && objectID != playerID) {
            // Handle special cases for death zone and boundaries
            if (objectID == deathZoneID) {
                handleDeathzone();
            }
            else {
                handleCollision(objectID);  // Handle collision for any other object with Collision property
            }
        }
    }
}

// Handle when the player enters the death zone
void Game::handleDeathzone() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    std::shared_ptr<RectProperty> deathzoneRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(deathZoneID, "Rect"));
    SDL_Rect deathRect = { deathzoneRect->x, deathzoneRect->y, deathzoneRect->w, deathzoneRect->h };

    std::shared_ptr<RectProperty> spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

    // If the player collides with the death zone, respawn at the spawn point
    if (SDL_HasIntersection(&playRect, &deathRect)) {
        playerRect->x = spawnpointRect->x;
        playerRect->y = spawnpointRect->y;
        playerVel->vy = 0.0f;  // Reset vertical velocity
        playerVel->vx = 0.0f;  // Reset horizontal velocity

        // Handle screen boundary adjustments when the player respawns
        for (int i = 0; i < rightScrollCount; i++) {
            // Adjust platform positions based on how far the player has scrolled
        }

        for (int i = 0; i < leftScrollCount; i++) {
            // Adjust platform positions based on how far the player has scrolled
        }

        rightScrollCount = 0;
        leftScrollCount = 0;
    }
}

// Handle player collisions with screen boundaries (left/right)
void Game::handleBoundaries() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<RectProperty> rightBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(rightBoundaryID, "Rect"));
    std::shared_ptr<RectProperty> leftBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(leftBoundaryID, "Rect"));
    std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    std::shared_ptr<RectProperty> platformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID2, "Rect"));
    std::shared_ptr<RectProperty> platformRect3 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID3, "Rect"));
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    std::shared_ptr<RectProperty> movingPlatformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));

    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };
    SDL_Rect rightRect = { rightBoundaryRect->x, rightBoundaryRect->y, rightBoundaryRect->w, rightBoundaryRect->h };
    SDL_Rect leftRect = { leftBoundaryRect->x, leftBoundaryRect->y, leftBoundaryRect->w, leftBoundaryRect->h };

    // Handle collision with right boundary
    if (SDL_HasIntersection(&playRect, &rightRect)) {
        playerRect->x = rightBoundaryRect->x - playerRect->w;  // Prevent player from moving past the right boundary
        // Adjust platform positions as the player moves right
    }

    // Handle collision with left boundary
    if (SDL_HasIntersection(&playRect, &leftRect)) {
        playerRect->x = leftBoundaryRect->x + playerRect->w;  // Prevent player from moving past the left boundary
        // Adjust platform positions as the player moves left
    }
}

// Update the game state, including player movement, platform movement, and collision checks
void Game::update() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update game objects (e.g., player position, platform movement)
    updateGameObjects();

    // Check for collisions with platforms, boundaries, and death zones
    checkCollisions();
}

// Update game object properties
void Game::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    // Update the player's position using its velocity
    playerRect->x += playerVel->vx;
    playerRect->y += playerVel->vy;

    // Apply simple gravity to the player
    if (playerRect->y < SCREEN_HEIGHT) {
        playerVel->vy += 1;  // Simulate gravity
        if (playerVel->vy > 10) { // Limit fall speed to terminal velocity
            playerVel->vy = 10;
        }
    }
    else {
        playerRect->y = SCREEN_HEIGHT;  // Clamp the player to the ground
        playerVel->vy = 0;  // Stop falling
    }

    // 3. Update moving platform position (horizontal)
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID, "Velocity"));

    if (movingPlatformVel) {
        movingPlatformRect->x += movingPlatformVel->vx;  // Update the position based on velocity
    }
    else {
        std::cerr << "Error: movingPlatformVel is null" << std::endl;
    }

    // Check for boundary collisions and reverse direction if needed
    if (movingPlatformRect->x <= 0 || movingPlatformRect->x >= SCREEN_WIDTH - movingPlatformRect->w) {
        movingPlatformVel->vx = -movingPlatformVel->vx;  // Reverse direction
    }

    // Update second moving platform (vertical movement)
    std::shared_ptr<RectProperty> movingPlatformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel2 = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID2, "Velocity"));

    movingPlatformRect2->y += movingPlatformVel2->vy;  // Update the position based on velocity
    if (movingPlatformRect2->y <= 0 || movingPlatformRect2->y >= SCREEN_HEIGHT - movingPlatformRect2->h) {  // Reverse direction
        movingPlatformVel2->vy = -movingPlatformVel2->vy;
    }
}

// Render game objects to the screen
void Game::render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);  // Blue background
    SDL_RenderClear(renderer);

    // Render static platforms and moving platforms
    renderPlatform(platformID);  // Render the first platform
    renderPlatform(platformID2);  // Render the second platform
    renderPlatform(platformID3);  // Render the third platform

    renderPlatform(movingPlatformID);  // Render the first moving platform
    renderPlatform(movingPlatformID2);  // Render the second moving platform

    // Render player character
    renderPlayer(playerID);

    // Render other players, adjusted by the camera offset
    for (const auto& player : allPlayers) {
        int id = player.first;
        if (id != clientId) {
            PlayerPosition pos = player.second;

            // Adjust the player position based on the camera position
            SDL_Rect otherPlayerRect = { pos.x - cameraX, pos.y - cameraY, 50, 50 };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green color for other players
            SDL_RenderFillRect(renderer, &otherPlayerRect);
        }
    }

    // Present the updated screen
    SDL_RenderPresent(renderer);
}

// Helper function to render platforms
void Game::renderPlatform(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));

    // Adjust the platform position based on the camera offset
    SDL_Rect platformRect = { rect->x - cameraX, rect->y - cameraY, rect->w, rect->h };

    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(platformID, "Render"));
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &platformRect);
}

// Helper function to render player
void Game::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Adjust player position based on camera offset
    SDL_Rect playerRect = { rect->x - cameraX, rect->y - cameraY, rect->w, rect->h };

    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(playerID, "Render"));
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &playerRect);
}