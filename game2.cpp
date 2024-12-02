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

Game2::Game2(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), quit(false), clientId(-1), playerLives(3), gameTimeline(nullptr, 1.0f)
{
    initGameObjects();
}

Game2::~Game2() {}

// Initialize game objects and register event handlers
void Game2::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();
    auto& eventManager = EventManager::getInstance();

    // Register input event handler
    eventManager.registerHandler(INPUT, [this](std::shared_ptr<Event> event) {
        auto inputEvent = std::static_pointer_cast<InputEvent>(event);
        handleInput(inputEvent->getObjectID(), inputEvent->getInputAction());
    });

    // Register collision event handler
    eventManager.registerHandler(COLLISION, [this](std::shared_ptr<Event> event) {
        auto collisionEvent = std::static_pointer_cast<CollisionEvent>(event);
        resolveCollision(collisionEvent->getObject1ID(), collisionEvent->getObject2ID());
    });

    // Register death event handler
    eventManager.registerHandler(DEATH, [this](std::shared_ptr<Event> event) {
        auto deathEvent = std::static_pointer_cast<DeathEvent>(event);
        handleDeath(deathEvent->getObjectID());
    });

    // Register spawn event handler
    eventManager.registerHandler(SPAWN, [this](std::shared_ptr<Event> event) {
        auto spawnEvent = std::static_pointer_cast<SpawnEvent>(event);
        handleSpawn(spawnEvent->getObjectID());
    });

    // Create player object and add properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH / 2 - 25, 1000, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(255, 51, 153));
    propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));
    propertyManager.addProperty(playerID, "Input", std::make_shared<InputProperty>(true, false));

    // Create boss object and add properties
    bossID = propertyManager.createObject();
    propertyManager.addProperty(bossID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH / 2 - 250, 100, 500, 200));
    propertyManager.addProperty(bossID, "Render", std::make_shared<RenderProperty>(0, 0, 0));

    // Create enemy objects and add properties
    int enemyX = SCREEN_WIDTH / 2 - 250;
    int enemyY = 100;
    for (int i = 1; i <= 25; ++i) {
        int enemyID = propertyManager.createObject();
        enemyX += 25;
        if (enemyX > (SCREEN_WIDTH / 2 + 150)) {
            enemyX = SCREEN_WIDTH / 2 - 250;
        }
        propertyManager.addProperty(enemyID, "Rect", std::make_shared<RectProperty>(enemyX, enemyY, 100, 100));
        propertyManager.addProperty(enemyID, "Render", std::make_shared<RenderProperty>(255, 128, 0));
        propertyManager.addProperty(enemyID, "Collision", std::make_shared<CollisionProperty>(true));
        propertyManager.addProperty(enemyID, "Velocity", std::make_shared<VelocityProperty>(0, 0));
        enemyIDs.push_back(enemyID);
    }

	// Create life objects and add properties
    for (int i = 1; i <= playerLives; ++i) {
		int lifeID = propertyManager.createObject();
		propertyManager.addProperty(lifeID, "Rect", std::make_shared<RectProperty>(100 * i, 50, 50, 50));
		propertyManager.addProperty(lifeID, "Render", std::make_shared<RenderProperty>(255, 0, 0));
		lives.push_back(lifeID);
    }

    // Create spawn point object and add properties
    spawnPointID = propertyManager.createObject();
    propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH / 2 - 25, 1000, 50, 50));

    // Create right boundary object and add properties
    rightBoundaryID = propertyManager.createObject();
    propertyManager.addProperty(rightBoundaryID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH - 500, 0, 50, SCREEN_HEIGHT));
    propertyManager.addProperty(rightBoundaryID, "Collision", std::make_shared<CollisionProperty>(true));

    // Create left boundary object and add properties
    leftBoundaryID = propertyManager.createObject();
    propertyManager.addProperty(leftBoundaryID, "Rect", std::make_shared<RectProperty>(500, 0, 50, SCREEN_HEIGHT));
    propertyManager.addProperty(leftBoundaryID, "Collision", std::make_shared<CollisionProperty>(true));
}

// Main game loop
void Game2::run() {
    // Main game loop
    while (!quit) {
        // Handle input events (keyboard, mouse, etc.)
        handleEvents();
        
        // Receive player positions from the server
        receivePlayerPositions();
        
        // Dispatch all queued events to their respective handlers
        EventManager::getInstance().dispatchEvents();
        
        // Update the game state (e.g., player movement, collision checks)
        update();
        
        // Render all game objects to the screen
        render();
        
        // Delay to control the frame rate (approximately 60 FPS)
        SDL_Delay(16);
    }
}

// Function to handle SDL events (keyboard, mouse, etc.)
void Game2::handleEvents() {
    // Poll for SDL events
    while (SDL_PollEvent(&e) != 0) {
        // Check if the quit event is triggered
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }

    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the RectProperty of the player
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    // Retrieve the VelocityProperty of the player
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    // Get the current state of the keyboard
    const Uint8* keystates = SDL_GetKeyboardState(NULL);

    // Check if the left arrow key is pressed
    if (keystates[SDL_SCANCODE_LEFT]) {
        // Raise an input event for moving left
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_LEFT, &gameTimeline));
    }
    // Check if the right arrow key is pressed
    else if (keystates[SDL_SCANCODE_RIGHT]) {
        // Raise an input event for moving right
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_RIGHT, &gameTimeline));
    }
    // If no arrow keys are pressed
    else {
        // Raise an input event for stopping
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, STOP, &gameTimeline));
    }

    // Send the player's movement update to the server
    sendMovementUpdate();
}

void Game2::reset() {
	// Get the instance of the PropertyManager
	auto& propertyManager = PropertyManager::getInstance();

	// Reset the positions of all enemies
	int enemyX = SCREEN_WIDTH / 2 - 250;
	int enemyY = 100;
	for (int spikeID : enemyIDs) {
		auto enemyRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spikeID, "Rect"));
		enemyRect->x = enemyX;
		enemyRect->y = enemyY;
		enemyX += 25;
		if (enemyX > (SCREEN_WIDTH / 2 + 150)) {
			enemyX = SCREEN_WIDTH / 2 - 250;
		}
		auto enemyVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(spikeID, "Velocity"));
		enemyVel->vy = 0;
		enemyVel->vx = 0;
	}
}

// Function to handle death events for game objects
void Game2::handleDeath(int objectID) {
    // Log the death event
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the RectProperty of the player
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));
    // Retrieve the VelocityProperty of the player
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    // Retrieve the RectProperty of the spawn point
    auto spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

    // Reset the player's position to the spawn point
    playerRect->x = spawnpointRect->x;
    playerRect->y = spawnpointRect->y;
    // Reset the player's velocity
    playerVel->vy = 0;
    playerVel->vx = 0;

    // Decrease the player's lives by one
    playerLives--;

	// Remove a life indicator object
	lives.pop_back();

	// Reset the game state if the player has no lives left
    reset();

    // If the player has no lives left, set the quit flag to true
    if (playerLives == 0) {
		quit = true;
    }

    // Raise a spawn event for the player
    EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(objectID, &gameTimeline));
}

// Function to handle spawn events for game objects
void Game2::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;
}

// Function to handle input events for player movement
void Game2::handleInput(int objectID, const InputAction& inputAction) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the VelocityProperty of the player
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    // Retrieve the InputProperty of the player
    auto playerInput = std::static_pointer_cast<InputProperty>(propertyManager.getProperty(objectID, "Input"));

    // Check the input action and update the player's velocity accordingly
    if (inputAction == MOVE_LEFT) {
        playerVel->vx = -10; // Move left
    }
    else if (inputAction == MOVE_RIGHT) {
        playerVel->vx = 10; // Move right
    }
    else if (inputAction == STOP) {
        playerVel->vx = 0; // Stop moving
    }
}

// Function to resolve collision between two objects
void Game2::resolveCollision(int obj1ID, int obj2ID) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();

    // Retrieve the RectProperty of the first object (player)
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(obj1ID, "Rect"));
    // Retrieve the VelocityProperty of the first object (player)
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(obj1ID, "Velocity"));
    // Retrieve the PhysicsProperty of the first object (player)
    auto playerPhysics = std::static_pointer_cast<PhysicsProperty>(propertyManager.getProperty(obj1ID, "Physics"));
    // Retrieve the InputProperty of the first object (player)
    auto playerInput = std::static_pointer_cast<InputProperty>(propertyManager.getProperty(obj1ID, "Input"));

    // Retrieve the RectProperty of the second object (enemy)
    auto enemyRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(obj2ID, "Rect"));

    // Check if the player is colliding with the enemy from the bottom
    if (playerRect->y + playerRect->h / 2 > enemyRect->y + enemyRect->h) {
        handleDeath(obj1ID); // Handle player death
    }
    // Check if the player is colliding with the enemy from the left
    else if (playerRect->x + playerRect->w / 2 < enemyRect->x) {
        handleDeath(obj1ID); // Handle player death
    }
    // Check if the player is colliding with the enemy from the right
    else if (playerRect->x + playerRect->w / 2 > enemyRect->x + enemyRect->w) {
        handleDeath(obj1ID); // Handle player death
    }
}

// Function to send the player's movement update to the server
void Game2::sendMovementUpdate() {
    // Create a message with the size of clientId and PlayerPosition
    zmq::message_t request(sizeof(clientId) + sizeof(PlayerPosition));

    // Create a PlayerPosition object to store the player's position
    PlayerPosition pos;
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the RectProperty of the player
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));

    // Set the player's position
    pos.x = playerRect->x;
    pos.y = playerRect->y;

    // Copy the clientId and player's position into the message
    memcpy(request.data(), &clientId, sizeof(clientId));
    memcpy(static_cast<char*>(request.data()) + sizeof(clientId), &pos, sizeof(pos));

    // Send the message to the server
    reqSocket.send(request, zmq::send_flags::none);

    // Receive the server's reply
    zmq::message_t reply;
    reqSocket.recv(reply);

    // If the clientId is -1, update it with the assigned playerId from the server
    if (clientId == -1) {
        memcpy(&clientId, reply.data(), sizeof(clientId));
        std::cout << "Received assigned playerId: " << clientId << std::endl;
    }
}

// Function to receive player positions from the server
void Game2::receivePlayerPositions() {
    zmq::message_t update;
    // Attempt to receive a message from the subscription socket without blocking
    zmq::recv_result_t recvResult = subSocket.recv(update, zmq::recv_flags::dontwait);

    // If a message was received
    if (recvResult) {
        // Clear the current list of all players
        allPlayers.clear();
        // Pointer to the data buffer of the received message
        char* buffer = static_cast<char*>(update.data());

        // Iterate through the buffer to extract player positions
        while (buffer < static_cast<char*>(update.data()) + update.size() - sizeof(PlayerPosition)) {
            int id;
            PlayerPosition pos;

            // Copy the player ID from the buffer
            memcpy(&id, buffer, sizeof(id));
            buffer += sizeof(id);
            // Copy the player position from the buffer
            memcpy(&pos, buffer, sizeof(pos));
            buffer += sizeof(pos);

            // Store the player position in the allPlayers map
            allPlayers[id] = pos;
        }
    }
}

// Handle collisions between the player and enemy objects
void Game2::handleCollision(int spikeID) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    
    // Retrieve the RectProperty of the player
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    // Create an SDL_Rect using the player's RectProperty
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    // Retrieve the RectProperty of the enemy
    auto enemyRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spikeID, "Rect"));
    // Create an SDL_Rect using the enemy's RectProperty
    SDL_Rect eneRect = { enemyRect->x, enemyRect->y, enemyRect->w, enemyRect->h };

    // Check for intersection between the player and enemy rectangles
    if (SDL_HasIntersection(&playRect, &eneRect)) {
        // Raise a collision event if there is an intersection
        EventManager::getInstance().raiseEvent(std::make_shared<CollisionEvent>(playerID, spikeID, &gameTimeline));
    }
}

// Check for collisions between game objects
void Game2::checkCollisions() {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve all properties from the PropertyManager
    const auto& allProperties = propertyManager.getAllProperties();

    // Iterate through all properties to check for collisions
    for (const auto& objectPair : allProperties) {
        int objectID = objectPair.first;

        // Check if the object has a Collision property and is not the player
        if (objectPair.second.count("Collision") && objectID != playerID) {
            // Handle collisions with screen boundaries
            if (objectID == leftBoundaryID || objectID == rightBoundaryID) {
                handleBoundaries();
            }
            // Handle collisions with other objects
            else {
                handleCollision(objectID);
            }
        }
    }
}

// Handle collisions with screen boundaries to prevent players from moving off-screen
void Game2::handleBoundaries() {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    
    // Retrieve the RectProperty of the player
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    
    // Retrieve the RectProperty of the right boundary
    std::shared_ptr<RectProperty> rightBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(rightBoundaryID, "Rect"));
    
    // Retrieve the RectProperty of the left boundary
    std::shared_ptr<RectProperty> leftBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(leftBoundaryID, "Rect"));

    // Create SDL_Rect structures for the player and boundaries
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };
    SDL_Rect rightRect = { rightBoundaryRect->x, rightBoundaryRect->y, rightBoundaryRect->w, rightBoundaryRect->h };
    SDL_Rect leftRect = { leftBoundaryRect->x, leftBoundaryRect->y, leftBoundaryRect->w, leftBoundaryRect->h };

    // Check for intersection with the right boundary
    if (SDL_HasIntersection(&playRect, &rightRect)) {
        // Prevent the player from moving beyond the right boundary
        playerRect->x = rightBoundaryRect->x - playerRect->w;
    }

    // Check for intersection with the left boundary
    if (SDL_HasIntersection(&playRect, &leftRect)) {
        // Prevent the player from moving beyond the left boundary
        playerRect->x = leftBoundaryRect->x + playerRect->w;
    }
}

// Update the overall game state
void Game2::update() {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();

    // Update the positions and velocities of game objects
    updateGameObjects();

    // Check for collisions between game objects
    checkCollisions();
}

// Update the positions and velocities of game objects
void Game2::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    playerRect->x += playerVel->vx;

    // Seed the random number generator
    std::srand(std::time(nullptr));

    // Select two random enemies
    int randomEnemy = std::rand() % enemyIDs.size();
    int chosenEnemyID = enemyIDs[randomEnemy];
    int randomEnemy2 = std::rand() % enemyIDs.size();
    int chosenEnemyID2 = enemyIDs[randomEnemy2];

    // Update velocities of the chosen enemies
    std::shared_ptr<VelocityProperty> enemyVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(chosenEnemyID, "Velocity"));
    enemyVel->vy = (std::rand() % 11) + 20;
    enemyVel->vx = (std::rand() % 11) - 5;

    std::shared_ptr<VelocityProperty> enemyVel2 = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(chosenEnemyID2, "Velocity"));
    enemyVel2->vy = (std::rand() % 11) + 20;
    enemyVel2->vx = (std::rand() % 11) - 5;

    // Update positions of all enemies based on their velocities
    for (int spikeID : enemyIDs) {
        std::shared_ptr<RectProperty> enemyRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spikeID, "Rect"));
        std::shared_ptr<VelocityProperty> enemyVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(spikeID, "Velocity"));
        enemyRect->y += enemyVel->vy;
        enemyRect->x += enemyVel->vx;
    }

    // Reset enemies that move off-screen
    for (int spikeID : enemyIDs) {
        std::shared_ptr<RectProperty> enemyRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spikeID, "Rect"));
        std::shared_ptr<VelocityProperty> enemyVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(spikeID, "Velocity"));
        if (enemyRect->y > SCREEN_HEIGHT) {
            enemyRect->y = 100;
            enemyRect->x = SCREEN_WIDTH / 2 - 250 + std::rand() % 401;
            enemyVel->vy = 0;
            enemyVel->vx = 0;
        }
    }
}

// Main render function
void Game2::render() {
    // Set the background color
    SDL_SetRenderDrawColor(renderer, 0, 51, 102, 255);
    // Clear the screen with the background color
    SDL_RenderClear(renderer);

    // Render all enemy objects
    for (int spikeID : enemyIDs) {
        renderObject(spikeID);
    }

	// Render all life objects
	for (int lifeID : lives) {
		renderObject(lifeID);
	}

    // Render the player object
    renderObject(playerID);

    // Render the boss object
    renderObject(bossID);

    // Render all other players
    for (const auto& player : allPlayers) {
        int id = player.first;
        if (id != clientId) {
            PlayerPosition pos = player.second;
            SDL_Rect otherPlayerRect = { pos.x, pos.y, 50, 50 };
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_RenderFillRect(renderer, &otherPlayerRect);
        }
    }

    // Present the rendered frame to the screen
    SDL_RenderPresent(renderer);
}

// Render a specific object based on its ID
void Game2::renderObject(int objectID) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    
    // Retrieve the RectProperty of the object
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));

    // Create an SDL_Rect using the object's RectProperty
    SDL_Rect objectRect = { rect->x, rect->y, rect->w, rect->h };

    // Retrieve the RenderProperty of the object
    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(objectID, "Render"));
    
    // Set the render draw color using the enemy's RenderProperty
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    
    // Render the enemy rectangle
    SDL_RenderFillRect(renderer, &objectRect);
}