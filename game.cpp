#include "game.h"
#include "PropertyManager.h" // For property-based model
#include "ThreadManager.h"
#include <iostream>
#include <cstring>


// Constructor for the Game class
Game::Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), quit(false), clientId(-1), cameraX(0), cameraY(0)
{
    initGameObjects();  // Initialize property-based objects (players, platforms, etc.)
}

// Destructor
Game::~Game() {}

// Initialize game objects (characters, platforms, etc.)
void Game::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Create player object with position and velocity properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(200, 850, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(98, 9, 176));
    propertyManager.addProperty(playerID, "Physics", std::make_shared<PhysicsProperty>(10.0f));
    propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create static platform objects
    platformID = propertyManager.createObject();
    propertyManager.addProperty(platformID, "Rect", std::make_shared<RectProperty>(50, 900, 400, 50));
    propertyManager.addProperty(platformID, "Render", std::make_shared<RenderProperty>(163, 11, 11));
    propertyManager.addProperty(platformID, "Collision", std::make_shared<CollisionProperty>(true));

    platformID2 = propertyManager.createObject();
    propertyManager.addProperty(platformID2, "Rect", std::make_shared<RectProperty>(450, 800, 400, 150));
    propertyManager.addProperty(platformID2, "Render", std::make_shared<RenderProperty>(201, 113, 12));
    propertyManager.addProperty(platformID2, "Collision", std::make_shared<CollisionProperty>(true));

    platformID3 = propertyManager.createObject();
    propertyManager.addProperty(platformID3, "Rect", std::make_shared<RectProperty>(1000, 650, 900, 50));
    propertyManager.addProperty(platformID3, "Render", std::make_shared<RenderProperty>(83, 145, 7));
    propertyManager.addProperty(platformID3, "Collision", std::make_shared<CollisionProperty>(true));

    // Create moving platform
    movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Rect", std::make_shared<RectProperty>(150, 500, 200, 50));
    propertyManager.addProperty(movingPlatformID, "Render", std::make_shared<RenderProperty>(0, 0, 255));
    propertyManager.addProperty(movingPlatformID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally

    // Create second moving platform (vertical movement)
    movingPlatformID2 = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID2, "Rect", std::make_shared<RectProperty>(2000, 150, 200, 50));  // Different initial position
    propertyManager.addProperty(movingPlatformID2, "Render", std::make_shared<RenderProperty>(186, 168, 7)); 
    propertyManager.addProperty(movingPlatformID2, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID2, "Velocity", std::make_shared<VelocityProperty>(0, 2));  // Moving vertically

    spawnPointID = propertyManager.createObject();
    propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(200, 850, 50, 50));

    // Create death zone (hidden object)
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH * -1, SCREEN_HEIGHT - 50, SCREEN_WIDTH * 3, 50));  // Just above the bottom of the screen
    propertyManager.addProperty(deathZoneID, "Collision", std::make_shared<CollisionProperty>(true));  // Enable collision detection

}

// Main game loop
void Game::run() {
    // Main thread handles event polling, input, and rendering
    while (!quit) {
        handleEvents();  // Handle input and events

        // Receive player positions from the server
        receivePlayerPositions();

        // Update game state if needed (but no longer based on time)
        update();

		// Update game objects (e.g., player position, platform movement)
        updateGameObjects();

        // Render the game
        render();

        // Cap frame rate to around 60 FPS
        SDL_Delay(16);
    }
}


// Handle events, including input
void Game::handleEvents() {

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        // Handle input for local player only
        auto& propertyManager = PropertyManager::getInstance();
        std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
        std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

        // Handle keyboard input
		const Uint8* keystates = SDL_GetKeyboardState(NULL); // Get the current state of the keyboard
		if (keystates[SDL_SCANCODE_LEFT]) { // Move left
            playerVel->vx = -5;

        }
		else if (keystates[SDL_SCANCODE_RIGHT]) { // Move right
            playerVel->vx = 5;
        }
		else { // Stop moving if no keys are pressed
            playerVel->vx = 0;
        }

		if (keystates[SDL_SCANCODE_UP] && playerVel->vy == 1) { // Only jump if player is on the ground
            playerVel->vy = -20.1;  // Jump
        }

        // Send only local player updates
        sendMovementUpdate();
    }

    updateCamera();  // Update camera based on player position
}

// Update camera to follow the player
void Game::updateCamera() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Center camera on the player
    cameraX = playerRect->x - (SCREEN_WIDTH / 2 - playerRect->w / 2);
    cameraY = playerRect->y - (SCREEN_HEIGHT / 2 - playerRect->h / 2);
}


// Send the current player's position to the server
void Game::sendMovementUpdate() {
    zmq::message_t request(sizeof(clientId) + sizeof(PlayerPosition));

    // Package the player ID and position
    PlayerPosition pos;
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

	// Get the player's position
    pos.x = playerRect->x; 
    pos.y = playerRect->y;

    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    // Send the data to the server
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

// Receive player positions from the server
void Game::receivePlayerPositions() {
    zmq::message_t update;
    zmq::recv_result_t recvResult = subSocket.recv(update, zmq::recv_flags::dontwait);

    if (recvResult) {
        allPlayers.clear();
        char* buffer = static_cast<char*>(update.data());

        // Deserialize player positions
        while (buffer < static_cast<char*>(update.data()) + update.size() - sizeof(PlayerPosition)) {
            int id;
            PlayerPosition pos;

            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            allPlayers[id] = pos;  // Update positions for all players
        }
    }
}


void Game::handleCollision(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    std::shared_ptr<PhysicsProperty> playerPhysics = std::static_pointer_cast<PhysicsProperty>(propertyManager.getProperty(playerID, "Physics"));
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h }; // This is just to check collision

    std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    SDL_Rect platRect = { platformRect->x, platformRect->y, platformRect->w, platformRect->h }; // This is just to check collision

    if (SDL_HasIntersection(&playRect, &platRect)) {
        if (playerRect->y + playerRect->h / 2 < platformRect->y) { // checks above platform
            playerVel->vy = 0.0f; // Stops downward velocity
            playerRect->y = platformRect->y - playerRect->h; // Positions entity on top of platform
        }
        if (playerRect->y + playerRect->h / 2 > platformRect->y + platformRect->h) { // check below platform
            playerVel->vy = playerPhysics->gravity; // Sets downward velocity to positive (jumping is just set velocityY to negative)
            playerRect->y = platformRect->y + platformRect->h; // Positions entity below platform
        }
        if (playerRect->x + playerRect->w / 2 < platformRect->x) { // Checks left of platform
            playerRect->x = platformRect->x - playerRect->w; // Positions entity to left of platform
        }
        if (playerRect->x + playerRect->w / 2 > platformRect->x + platformRect->w) { // Checks right of platform
            playerRect->x = platformRect->x + platformRect->w; // Positions entity to right of platform
        }
    }
}

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

void Game::handleDeathzone() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
	SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h }; // Create a rect for the player

    std::shared_ptr<RectProperty> deathzoneRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(deathZoneID, "Rect"));
	SDL_Rect deathRect = { deathzoneRect->x, deathzoneRect->y, deathzoneRect->w, deathzoneRect->h }; // Create a rect for the deathzone

    std::shared_ptr<RectProperty> spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

	// Check if player is in the death zone
    if (SDL_HasIntersection(&playRect, &deathRect)) {
		// Reset player position to spawn point
        playerRect->x = spawnpointRect->x;
        playerRect->y = spawnpointRect->y;
		// Reset player velocity
        playerVel->vy = 0.0f;
        playerVel->vx = 0.0f;
    }
}

// Update the game state, including player movement, platform movement, and collision checks
void Game::update() {
    auto& propertyManager = PropertyManager::getInstance();

    // 1. Update game objects (e.g., player position, platform movement)
    updateGameObjects();

    // 2. Check for collisions with platforms, boundaries, and death zones
    checkCollisions();
}

// Update game object properties
void Game::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // 1. Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    playerRect->x += playerVel->vx;
    playerRect->y += playerVel->vy;

    // 2. Apply simple gravity to the player
    if (playerRect->y < SCREEN_HEIGHT) {
        playerVel->vy += 1;  // Gravity effect
		if (playerVel->vy > 10) { // Terminal velocity
			playerVel->vy = 10;
        }
    }
    else {
        playerRect->y = SCREEN_HEIGHT;  // Clamp to ground
        playerVel->vy = 0;  // Stop falling when hitting the ground
    }

    // 3. Update moving platform position
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID, "Velocity"));

    if (movingPlatformVel) {
        movingPlatformRect->x += movingPlatformVel->vx;
    }
    else {
        std::cerr << "Error: movingPlatformVel is null" << std::endl;
    }

    if (movingPlatformRect->x <= 0 || movingPlatformRect->x >= SCREEN_WIDTH - movingPlatformRect->w) {  // Bounce within screen bounds
        movingPlatformVel->vx = -movingPlatformVel->vx;
    }

    // 4. Update second moving platform (vertical movement)
    std::shared_ptr<RectProperty> movingPlatformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel2 = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID2, "Velocity"));

    movingPlatformRect2->y += movingPlatformVel2->vy;
    if (movingPlatformRect2->y <= 0 || movingPlatformRect2->y >= SCREEN_HEIGHT - movingPlatformRect2->h) {  // Bounce within screen bounds
        movingPlatformVel2->vy = -movingPlatformVel2->vy;
    }
}

// Render game objects to the screen
void Game::render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 96, 128, 255, 255);
    SDL_RenderClear(renderer);

    // Render static platforms and moving platforms
    renderPlatform(platformID);
    renderPlatform(platformID2);  // Render second platform
    renderPlatform(platformID3);  // Render third platform

    renderPlatform(movingPlatformID);  // Render moving platform
    renderPlatform(movingPlatformID2);  // Render second moving platform

    // Render player character
    renderPlayer(playerID);

    // Render other players, adjusted by the camera offset
    for (const auto& player : allPlayers) {
        int id = player.first;
        if (id != clientId) {
            PlayerPosition pos = player.second;

            SDL_Rect otherPlayerRect = { pos.x - cameraX, pos.y - cameraY, 50, 50 };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for other players
            SDL_RenderFillRect(renderer, &otherPlayerRect);
        }
    }

    SDL_RenderPresent(renderer);  // Present the updated screen
}

// Helper function to render platforms
void Game::renderPlatform(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));

    // Adjust platform position by camera offset
    SDL_Rect platformRect = { rect->x - cameraX, rect->y - cameraY, rect->w, rect->h };

    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(platformID, "Render"));
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &platformRect);
}

// Helper function to render player
void Game::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Adjust player position by camera offset
    SDL_Rect playerRect = { rect->x - cameraX, rect->y - cameraY, rect->w, rect->h };

    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(playerID, "Render"));
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &playerRect);
}