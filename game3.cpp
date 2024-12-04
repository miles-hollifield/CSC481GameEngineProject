#include "game3.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include <iostream>
#include <cstring>
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"
#include "InputEvent.h"
#include "CollisionEvent.h"

Game3::Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), clientId(-1), playerLives(5), font(nullptr), timeTexture(nullptr), gameTimeline(nullptr, 1.0f)
{
    TTF_Init();  // Initialize SDL_ttf for text rendering
    font = TTF_OpenFont("./fonts/PixelPowerline-9xOK.ttf", 24);  // Load a font for rendering text

    initGameObjects();
}

Game3::~Game3() {
    TTF_CloseFont(font);  // Close the font
    SDL_DestroyTexture(timeTexture);  // Destroy the score texture
    TTF_Quit();  // Quit SDL_ttf
}

// Initialize game objects and register event handlers
void Game3::initGameObjects() {
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

    // Spawn point coordinates
    std::srand(std::time(nullptr));
    int spawnX = (std::rand() % (GRID_WIDTH)) * 100 + 15;
    int spawnY = (std::rand() % (GRID_HEIGHT)) * 100 + 15;

    // Create player object and add properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(spawnX + 15, spawnY + 15, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(255, 51, 153));
    propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));
    propertyManager.addProperty(playerID, "Input", std::make_shared<InputProperty>(true, false));

    // Create horizontal wall objects
    int wallW = 100;
    int wallH = 10;
    for (int i = 0; i < GRID_HEIGHT + 1; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            if (rand() % 100 < 50 || i == 0 || i == GRID_HEIGHT) {
                int wallX = j * 100;
                int wallY = i * 100;
                int wallID = propertyManager.createObject();
                propertyManager.addProperty(wallID, "Rect", std::make_shared<RectProperty>(wallX, wallY, wallW, wallH));
                propertyManager.addProperty(wallID, "Render", std::make_shared<RenderProperty>(255, 128, 0));
                propertyManager.addProperty(wallID, "Collision", std::make_shared<CollisionProperty>(true));
                horizWallIDs.push_back(wallID);
            }
        }
    }

    // Create vertical wall objects
    wallW = 10;
    wallH = 100;
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH + 1; j++) {
            if (rand() % 100 < 50 || j == 0 || j == GRID_WIDTH) {
                int wallX = j * 100;
                int wallY = i * 100;
                int wallID = propertyManager.createObject();
                propertyManager.addProperty(wallID, "Rect", std::make_shared<RectProperty>(wallX, wallY, wallW, wallH));
                propertyManager.addProperty(wallID, "Render", std::make_shared<RenderProperty>(255, 128, 0));
                propertyManager.addProperty(wallID, "Collision", std::make_shared<CollisionProperty>(true));
                vertWallIDs.push_back(wallID);
            }
        }
    }

    // Create life objects and add properties
    for (int i = 1; i <= playerLives; ++i) {
        int lifeID = propertyManager.createObject();
        propertyManager.addProperty(lifeID, "Rect", std::make_shared<RectProperty>(100 * i, 1020, 50, 50));
        propertyManager.addProperty(lifeID, "Render", std::make_shared<RenderProperty>(255, 0, 0));
        lives.push_back(lifeID);
    }

    // Create spawn point object and add properties
    spawnPointID = propertyManager.createObject();
    propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(spawnX, spawnY, 75, 75));
    propertyManager.addProperty(spawnPointID, "Render", std::make_shared<RenderProperty>(0, 0, 255));

    // Create finish point object and add properties
    //std::srand(std::time(nullptr));
    int finishX = (std::rand() % (GRID_WIDTH)) * 100 + 15;
    int finishY = (std::rand() % (GRID_HEIGHT)) * 100 + 15;
    finishPointID = propertyManager.createObject();
    propertyManager.addProperty(finishPointID, "Rect", std::make_shared<RectProperty>(finishX, finishY, 75, 75));
    propertyManager.addProperty(finishPointID, "Render", std::make_shared<RenderProperty>(0, 255, 0));
    propertyManager.addProperty(finishPointID, "Collision", std::make_shared<CollisionProperty>(true));
}

// Main game loop
void Game3::run() {
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
void Game3::handleEvents() {
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
    else if (keystates[SDL_SCANCODE_DOWN]) {
        // Raise an input event for moving down
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_DOWN, &gameTimeline));
    }
    else if (keystates[SDL_SCANCODE_UP]) {
        // Raise an input event for moving up
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, MOVE_UP, &gameTimeline));
    }
    // If no arrow keys are pressed
    else {
        // Raise an input event for stopping
        EventManager::getInstance().raiseEvent(std::make_shared<InputEvent>(playerID, STOP, &gameTimeline));
    }

    // Send the player's movement update to the server
    sendMovementUpdate();
}

// Function to handle death events for game objects
void Game3::handleDeath(int objectID) {
    // Log the death event
    std::cout << "Death event triggered for object ID: " << objectID << std::endl;

    // Decrease the player's lives by one
    playerLives--;

    // Remove a life indicator object
    lives.pop_back();

    // If the player has no lives left, set the quit flag to true
    if (playerLives == 0) {
        quit = true;
    }

    // Raise a spawn event for the player
    EventManager::getInstance().raiseEvent(std::make_shared<SpawnEvent>(objectID, &gameTimeline));
}

// Function to handle spawn events for game objects
void Game3::handleSpawn(int objectID) {
    std::cout << "Spawn event triggered for object ID: " << objectID << std::endl;

    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the RectProperty of the player
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));
    // Retrieve the VelocityProperty of the player
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    // Retrieve the RectProperty of the spawn point
    auto spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

    SpawnEventData spawnData = sendSpawnEvent(objectID, spawnpointRect->x, spawnpointRect->y); // Send spawn event to server

    // Reset the player's position to the spawn point
    playerRect->x = spawnData.spawnX + 15;
    playerRect->y = spawnData.spawnY + 15;
    // Reset the player's velocity
    playerVel->vy = 0;
    playerVel->vx = 0;
}

SpawnEventData Game3::sendSpawnEvent(int objectID, int spawnX, int spawnY) {
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

// Function to handle input events for player movement
void Game3::handleInput(int objectID, const InputAction& inputAction) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve the VelocityProperty of the player
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(objectID, "Velocity"));
    // Retrieve the InputProperty of the player
    auto playerInput = std::static_pointer_cast<InputProperty>(propertyManager.getProperty(objectID, "Input"));

    // Check the input action and update the player's velocity accordingly
    if (inputAction == MOVE_LEFT) {
        playerVel->vx = -8; // Move left
    }
    else if (inputAction == MOVE_RIGHT) {
        playerVel->vx = 8; // Move right
    }
    else if (inputAction == MOVE_DOWN) {
        playerVel->vy = 8; // Move down
    }
    else if (inputAction == MOVE_UP) {
        playerVel->vy = -8; // Move up
    }
    else if (inputAction == STOP) {
        playerVel->vx = 0; // Stop moving
        playerVel->vy = 0;
    }
}

// Function to resolve collision between two objects
void Game3::resolveCollision(int obj1ID, int obj2ID) {
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

    // Retrieve the RectProperty of the second object (wall)
    auto wallRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(obj2ID, "Rect"));

    // Check if the player is colliding with the wall from the bottom
    if (playerRect->y + playerRect->h / 2 > wallRect->y + wallRect->h) {
        // Raise death event
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(obj1ID, &gameTimeline));
    }
    // Check if the player is colliding with the wall from the left
    else if (playerRect->x + playerRect->w / 2 < wallRect->x) {
        // Raise death event
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(obj1ID, &gameTimeline));
    }
    // Check if the player is colliding with the wall from the right
    else if (playerRect->x + playerRect->w / 2 > wallRect->x + wallRect->w) {
        // Raise death event
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(obj1ID, &gameTimeline));
    }
    // Check if the player is colliding with the wall from the top
    else if (playerRect->y + playerRect->h / 2 < wallRect->y) {
        // Raise death event
        EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(obj1ID, &gameTimeline));
    }

    // Remove the wall that was collided with if it is not on the border
    auto wallCollision = std::static_pointer_cast<CollisionProperty>(propertyManager.getProperty(obj2ID, "Collision"));

	// Check if the wall is a horizontal wall and not on the top or bottom border
    if (std::find(horizWallIDs.begin(), horizWallIDs.end(), obj2ID) != horizWallIDs.end() && wallRect->y != 0 && wallRect->y != GRID_HEIGHT * 100) {
        wallCollision->hasCollision = false;
        horizWallIDs.erase(std::remove(horizWallIDs.begin(), horizWallIDs.end(), obj2ID), horizWallIDs.end());
    } // Check if the wall is a vertical wall and not on the left or right border
	else if (std::find(vertWallIDs.begin(), vertWallIDs.end(), obj2ID) != vertWallIDs.end() && wallRect->x != 0 && wallRect->x != GRID_WIDTH * 100) {
        wallCollision->hasCollision = false;
        vertWallIDs.erase(std::remove(vertWallIDs.begin(), vertWallIDs.end(), obj2ID), vertWallIDs.end());
    }
    
}

// Function to send the player's movement update to the server
void Game3::sendMovementUpdate() {
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
void Game3::receivePlayerPositions() {
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

// Handle collisions between the player and wall objects
void Game3::handleCollision(int wallID) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();

    // Retrieve the RectProperty of the player
    auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    // Create an SDL_Rect using the player's RectProperty
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    // Retrieve the RectProperty of the wall
    auto wallRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(wallID, "Rect"));
    // Create an SDL_Rect using the wall's RectProperty
    SDL_Rect eneRect = { wallRect->x, wallRect->y, wallRect->w, wallRect->h };

    // Check for intersection between the player and wall rectangles
    if (SDL_HasIntersection(&playRect, &eneRect)) {
        if (wallID == finishPointID) {
            // Handle collisions with the finish point
            std::cout << "Player reached the finish point!" << std::endl;
            quit = true;
        }
        // Raise a collision event if there is an intersection
        EventManager::getInstance().raiseEvent(std::make_shared<CollisionEvent>(playerID, wallID, &gameTimeline));
    }
}

// Check for collisions between game objects
void Game3::checkCollisions() {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();
    // Retrieve all properties from the PropertyManager
    const auto& allProperties = propertyManager.getAllProperties();

    // Iterate through all properties to check for collisions
    for (const auto& objectPair : allProperties) {
        int objectID = objectPair.first;

        // Check if the object has a Collision property and is not the player
        if (objectPair.second.count("Collision") && objectID != playerID) {
            // Retrieve the CollisionProperty of the object
            auto collision = std::static_pointer_cast<CollisionProperty>(objectPair.second.at("Collision"));
            // Check if the object has collision enabled
            if (collision->hasCollision) {
                // Handle collisions with other objects
                handleCollision(objectID);
            }
        }
    }
}

// Update the overall game state
void Game3::update() {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();

    // Update the positions and velocities of game objects
    updateGameObjects();

    // Check for collisions between game objects
    checkCollisions();
}

// Update the positions and velocities of game objects
void Game3::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));
    playerRect->x += playerVel->vx;
    playerRect->y += playerVel->vy;
}

// Main render function
void Game3::render() {
    // Set the background color
    SDL_SetRenderDrawColor(renderer, 0, 51, 102, 255);
    // Clear the screen with the background color
    SDL_RenderClear(renderer);

    // Render all wall objects
    for (int wallID : horizWallIDs) {
        renderObject(wallID);
    }
	for (int wallID : vertWallIDs) {
		renderObject(wallID);
	}

    // Render all life objects
    for (int lifeID : lives) {
        renderObject(lifeID);
    }

    renderObject(spawnPointID);

    renderObject(finishPointID);

    renderTime();

    // Render the player object
    renderObject(playerID);

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
void Game3::renderObject(int objectID) {
    // Get the instance of the PropertyManager
    auto& propertyManager = PropertyManager::getInstance();

    // Retrieve the RectProperty of the object
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(objectID, "Rect"));

    // Create an SDL_Rect using the object's RectProperty
    SDL_Rect objectRect = { rect->x, rect->y, rect->w, rect->h };

    // Retrieve the RenderProperty of the object
    std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(objectID, "Render"));

    // Set the render draw color using the object's RenderProperty
    SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);

    // Render the object rectangle
    SDL_RenderFillRect(renderer, &objectRect);
}

void Game3::renderTime() {
    // Check if the time texture already exists and destroy it if it does
    if (timeTexture) {
        SDL_DestroyTexture(timeTexture);
        timeTexture = nullptr;
    }
    // Create the time text string
    std::string timeText = "Time: " + std::to_string(gameTimeline.getTime() / 1000);

    // Set the text color to white
    SDL_Color textColor = { 255, 255, 255 };

    // Render the text surface using the font and text color
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, timeText.c_str(), textColor);

    // Create a texture from the rendered text surface
    timeTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    // Free the text surface as it is no longer needed
    SDL_FreeSurface(textSurface);

    // Set the position of the time on the screen
    timeRect.x = 1800;
    timeRect.y = 1020;

    // Query the texture to get its width and height
    SDL_QueryTexture(timeTexture, NULL, NULL, &timeRect.w, &timeRect.h);

    // Copy the time texture to the renderer at the specified position
    SDL_RenderCopy(renderer, timeTexture, NULL, &timeRect);
}