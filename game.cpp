#include "game.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include <iostream>
#include <cstring>
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"
#include "InputEvent.h"
#include "CollisionEvent.h"


// Constructor for the Game class
Game::Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), clientId(-1), cameraX(0), cameraY(0), gameTimeline(nullptr, 1.0f)
{
    // Initialize game objects, such as players, platforms, etc.
    initGameObjects();
}

// Destructor
Game::~Game() {}

// Initialize game objects (characters, platforms, etc.)
void Game::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();
    auto& eventManager = EventManager::getInstance();

    // Register event handlers
    eventManager.registerHandler(INPUT, [this](std::shared_ptr<Event> event) {
        auto inputEvent = std::static_pointer_cast<InputEvent>(event);
        handleInput(inputEvent->getObjectID(), inputEvent->getInputAction());
        });

    eventManager.registerHandler(COLLISION, [this](std::shared_ptr<Event> event) {
        auto collisionEvent = std::static_pointer_cast<CollisionEvent>(event);
        resolveCollision(collisionEvent->getObject1ID(), collisionEvent->getObject2ID());
        });

    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
        });

    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
        });

    // Create player object and set its properties (position, render color, physics, etc.)
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(200, 850, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(98, 9, 176));
    propertyManager.addProperty(playerID, "Physics", std::make_shared<PhysicsProperty>(10));  // Gravity
    propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true)); // Enable collision
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0)); // Initial velocity
    propertyManager.addProperty(playerID, "Input", std::make_shared<InputProperty>(true, false)); // Input property

    // Create static platforms with different sizes and positions
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

    // Create a horizontally moving platform
    movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Rect", std::make_shared<RectProperty>(150, 500, 200, 50));
    propertyManager.addProperty(movingPlatformID, "Render", std::make_shared<RenderProperty>(0, 0, 255));
    propertyManager.addProperty(movingPlatformID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally
    std::cout << "Moving Platform 1 Velocity initialized" << std::endl;

    // Create a vertically moving platform
    movingPlatformID2 = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID2, "Rect", std::make_shared<RectProperty>(2000, 150, 200, 50));  // Different position
    propertyManager.addProperty(movingPlatformID2, "Render", std::make_shared<RenderProperty>(186, 168, 7));  // Orange color
    propertyManager.addProperty(movingPlatformID2, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID2, "Velocity", std::make_shared<VelocityProperty>(0, 2));  // Moving vertically
    std::cout << "Moving Platform 2 Velocity initialized" << std::endl;

    // Create a spawn point for the player
    spawnPointID = propertyManager.createObject();
    propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(200, 850, 50, 50));

    // Create a death zone at the bottom of the screen
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH * -1, SCREEN_HEIGHT - 50, SCREEN_WIDTH * 3, 50));  // Near the bottom
    propertyManager.addProperty(deathZoneID, "Collision", std::make_shared<CollisionProperty>(true));  // Enable collision for the death zone
}

// Main game loop
void Game::run() {
    // Main game loop that handles events, updates, and rendering
    while (!quit) {
        handleEvents();  // Handle input and events
        receivePlayerPositions();  // Receive other players' positions from the server
        EventManager::getInstance().dispatchEvents();
        update();  // Update the game state (e.g., player movement, collision detection)
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
    }

    // Handle input for the local player
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    // Handle keyboard input for player movement
    const Uint8* keystates = SDL_GetKeyboardState(NULL);

    if (keystates[SDL_SCANCODE_LEFT]) {
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_LEFT, &gameTimeline));
    }
    else if (keystates[SDL_SCANCODE_RIGHT]) {
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_RIGHT, &gameTimeline));
    }
    else {
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, STOP, &gameTimeline));
    }

    if (keystates[SDL_SCANCODE_UP] && playerVel->vy == 1) {
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, JUMP, &gameTimeline));
    }

    // Send the player's movement update to the server
    sendMovementUpdate();

    // Update camera to follow the player
    updateCamera();
}


void Game::handleDeath(int objectID) {
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(objectID, &gameTimeline));
}

void Game::handleSpawn(int objectID) {
    // Logic for handling spawn (e.g., setting player to a new position)
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;

    auto& propertyManager = PropertyManager::getInstance();
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    auto spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

	SpawnEventData spawnData = sendSpawnEvent(objectID, spawnpointRect->x, spawnpointRect->y); // Send spawn event to server

    // Reset player position to a spawnpoint based on the server response
    playerRect->x = spawnData.spawnX;
    playerRect->y = spawnData.spawnY;
    playerVel->vy = 0;  // Reset vertical velocity
    playerVel->vx = 0;  // Reset horizontal velocity
}

SpawnEventData Game::sendSpawnEvent(int objectID, int spawnX, int spawnY) {
	zmq::message_t request(sizeof(clientId) + sizeof(SpawnEventData));  // Create a message buffer for the request

	SpawnEventData spawnData; // Create a SpawnEventData struct to hold spawn position

    spawnData.spawnX = spawnX;
	spawnData.spawnY = spawnY;

	// Copy the client ID and spawn position into the request
	memcpy(request.data(), &clientId, sizeof(clientId));
	memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &spawnData, sizeof(SpawnEventData));

	// Send the request to the server
	eventReqSocket.send(request, zmq::send_flags::none);

	// Receive acknowledgment from the server
	zmq::message_t reply;
	eventReqSocket.recv(reply);

	// Deserialize the spawn data from the received data
	SpawnEventData spawnResponse;
	memcpy(&spawnResponse, static_cast<char*>(reply.data()) + sizeof(clientId), sizeof(spawnResponse));

	return spawnResponse;
}

void Game::handleInput(int objectID, const InputAction& inputAction) {
    auto& propertyManager = PropertyManager::getInstance();
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    auto playerInput = std::static_pointer_cast<InputProperty>(propertyManager.getProperty(objectID, "Input"));

    if (inputAction == MOVE_LEFT) {
        playerVel->vx = -5;
    }
    else if (inputAction == MOVE_RIGHT) {
        playerVel->vx = 5;
    }
    else if (inputAction == JUMP) {
        if (playerInput->isJumping == false && playerVel->vy == 1) {
            playerInput->isJumping = true;
            playerVel->vy = -20; // Jump velocity
        }
    }
    else if (inputAction == STOP) {
        playerVel->vx = 0;
    }
}

void Game::resolveCollision(int obj1ID, int obj2ID) {
    auto& propertyManager = PropertyManager::getInstance();

    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(obj1ID, "Rect"));
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(obj1ID, "Velocity"));
    auto playerPhysics = std::static_pointer_cast<PhysicsProperty>(propertyManager.getProperty(obj1ID, "Physics"));
    auto playerInput = std::static_pointer_cast<InputProperty>(propertyManager.getProperty(obj1ID, "Input"));

    auto platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(obj2ID, "Rect"));

    // Implement collision resolution logic
    if (playerRect->y + playerRect->h / 2 < platformRect->y) { // Player is above the platform
        playerVel->vy = 0;  // Stop downward velocity
        playerRect->y = platformRect->y - playerRect->h;  // Position the player on top of the platform
        playerInput->isJumping = false;
    }
    else if (playerRect->y + playerRect->h / 2 > platformRect->y + platformRect->h) { // Player is below the platform
        playerVel->vy = playerPhysics->gravity;  // Set downward velocity to simulate falling
        playerRect->y = platformRect->y + platformRect->h;  // Position the player below the platform
    }
    else if (playerRect->x + playerRect->w / 2 < platformRect->x) { // Player is to the left of the platform
        playerRect->x = platformRect->x - playerRect->w;  // Position the player to the left
    }
    else if (playerRect->x + playerRect->w / 2 > platformRect->x + platformRect->w) { // Player is to the right of the platform
        playerRect->x = platformRect->x + platformRect->w;  // Position the player to the right
    }
}



// Update the camera to follow the player's movement
void Game::updateCamera() {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Center the camera on the player
    cameraX = playerRect->x - (SCREEN_WIDTH / 2 - playerRect->w / 2);
    cameraY = playerRect->y - (SCREEN_HEIGHT / 2 - playerRect->h / 2);
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
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    auto platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    SDL_Rect platRect = { platformRect->x, platformRect->y, platformRect->w, platformRect->h };

    // Check for intersection between player and platform
    if (SDL_HasIntersection(&playRect, &platRect)) {
        EventManager::getInstance().raiseEvent(std::make_shared<CollisionEvent>(playerID, platformID, &gameTimeline));
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
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    auto deathzoneRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(deathZoneID, "Rect"));
    SDL_Rect deathRect = { deathzoneRect->x, deathzoneRect->y, deathzoneRect->w, deathzoneRect->h };

    // If the player collides with the death zone, raise a DeathEvent
    if (SDL_HasIntersection(&playRect, &deathRect)) {
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(playerID, &gameTimeline));
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
    SDL_SetRenderDrawColor(renderer, 96, 128, 255, 255);  // Blue background
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