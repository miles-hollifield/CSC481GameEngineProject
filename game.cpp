#include "game.h"
#include "PropertyManager.h"
#include "SDL2/SDL.h"
#include <iostream>
#include <memory>
#include <zmq.hpp>
#include "defs.h"

// Constructor
Game::Game(SDL_Renderer* renderer)
    : renderer(renderer), quit(false)
{
    initGameObjects();
}

// Destructor
Game::~Game() {}

// Initialize game objects (characters, platforms, etc.)
void Game::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Create player object with position and velocity properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(100, 400, 50, 50));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(255, 0, 0));
	propertyManager.addProperty(playerID, "Physics", std::make_shared<PhysicsProperty>(10.0f));
	propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true));
	propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create static platform objects
    platformID = propertyManager.createObject();
    propertyManager.addProperty(platformID, "Rect", std::make_shared<RectProperty>(50, 500, 200, 50));
	propertyManager.addProperty(platformID, "Render", std::make_shared<RenderProperty>(0, 255, 0));
	propertyManager.addProperty(platformID, "Collision", std::make_shared<CollisionProperty>(true));

    platformID2 = propertyManager.createObject();
    propertyManager.addProperty(platformID2, "Rect", std::make_shared<RectProperty>(250, 600, 200, 50));
	propertyManager.addProperty(platformID2, "Render", std::make_shared<RenderProperty>(0, 0, 255));
	propertyManager.addProperty(platformID2, "Collision", std::make_shared<CollisionProperty>(true));

    platformID3 = propertyManager.createObject();
    propertyManager.addProperty(platformID3, "Rect", std::make_shared<RectProperty>(450, 700, 400, 50));
	propertyManager.addProperty(platformID3, "Render", std::make_shared<RenderProperty>(0, 255, 255));
	propertyManager.addProperty(platformID3, "Collision", std::make_shared<CollisionProperty>(true));

    // Create moving platform
    movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Rect", std::make_shared<RectProperty>(150, 900, 200, 50));
	propertyManager.addProperty(movingPlatformID, "Render", std::make_shared<RenderProperty>(255, 255, 0));
	propertyManager.addProperty(movingPlatformID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally

    // Create second moving platform (vertical movement)
    movingPlatformID2 = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID2, "Rect", std::make_shared<RectProperty>(300, 200, 200, 50));  // Different initial position
    propertyManager.addProperty(movingPlatformID2, "Render", std::make_shared<RenderProperty>(255, 165, 0));  // Different color (orange)
    propertyManager.addProperty(movingPlatformID2, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID2, "Velocity", std::make_shared<VelocityProperty>(0, 2));  // Moving vertically


	spawnPointID = propertyManager.createObject();
	propertyManager.addProperty(spawnPointID, "Rect", std::make_shared<RectProperty>(100, 400, 50, 50));

    // Create death zone (hidden object)
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Rect", std::make_shared<RectProperty>(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50));  // Just above the bottom of the screen
	propertyManager.addProperty(deathZoneID, "Collision", std::make_shared<CollisionProperty>(true));  // Enable collision detection

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
void Game::run(zmq::socket_t& dealerSocket) {
    while (!quit) {
        handleEvents(dealerSocket);  // Handle player input and send data to the server
        updateGameObjects();
        checkCollisions();
        render();
        SDL_Delay(16);  // Cap frame rate
    }
}

// Handle player input, SDL events, and send updates to the server
void Game::handleEvents(zmq::socket_t& dealerSocket) {
    SDL_Event e;
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        // Keyboard input to control player movement
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        PlayerPosition newPosition = { playerRect->x, playerRect->y };

        if (keystates[SDL_SCANCODE_LEFT]) {
            playerVel->vx = -5;
            newPosition.x -= 5;
        }
        if (keystates[SDL_SCANCODE_RIGHT]) {
            playerVel->vx = 5;
            newPosition.x += 5;
        }
        if (keystates[SDL_SCANCODE_UP] && playerVel->vy == 0) {
            playerVel->vy = -15;
            newPosition.y -= 15;
        }

        // Send updated position to the server if it changed
        if (newPosition.x != playerRect->x || newPosition.y != playerRect->y) {
            playerRect->x = newPosition.x;
            playerRect->y = newPosition.y;

            zmq::message_t request(sizeof(PlayerPosition));
            memcpy(request.data(), &newPosition, sizeof(PlayerPosition));

            try {
                // Send position update (no need to wait for a reply in Dealer)
                dealerSocket.send(request, zmq::send_flags::none);
            }
            catch (const zmq::error_t& ex) {
                std::cerr << "Failed to send position: " << ex.what() << std::endl;
            }
        }
    }
}

// Update other players' positions based on server data
void Game::updatePlayerPosition(int playerID, const PlayerPosition& pos) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    playerRect->x = pos.x;
    playerRect->y = pos.y;
}

// Update platform positions based on server data
void Game::updatePlatformPosition(const PlayerPosition& pos) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    platformRect->x = pos.x;
    platformRect->y = pos.y;
}

void Game::updateVerticalPlatformPosition(const PlayerPosition& pos) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> platformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));
    platformRect2->x = pos.x;
    platformRect2->y = pos.y;
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
            else if (objectID == rightBoundaryID || objectID == leftBoundaryID) {
                handleBoundaries();
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
    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };

    std::shared_ptr<RectProperty> deathzoneRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(deathZoneID, "Rect"));
    SDL_Rect deathRect = { deathzoneRect->x, deathzoneRect->y, deathzoneRect->w, deathzoneRect->h };

    std::shared_ptr<RectProperty> spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));

    std::shared_ptr<RectProperty> rightBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(rightBoundaryID, "Rect"));
    std::shared_ptr<RectProperty> leftBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(leftBoundaryID, "Rect"));
    std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    std::shared_ptr<RectProperty> platformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID2, "Rect"));
    std::shared_ptr<RectProperty> platformRect3 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID3, "Rect"));
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
	std::shared_ptr<RectProperty> movingPlatformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));

    if (SDL_HasIntersection(&playRect, &deathRect)) {
        playerRect->x = spawnpointRect->x;
        playerRect->y = spawnpointRect->y;
        playerVel->vy = 0.0f;
		playerVel->vx = 0.0f;
        
        for (int i = 0; i < rightScrollCount; i++) {
            platformRect->x = platformRect->x + playerRect->w;
            platformRect2->x = platformRect2->x + playerRect->w;
            platformRect3->x = platformRect3->x + playerRect->w;
            movingPlatformRect->x = movingPlatformRect->x + playerRect->w;
			movingPlatformRect2->x = movingPlatformRect2->x + playerRect->w;
        }

        for (int i = 0; i < leftScrollCount; i++) {
            platformRect->x = platformRect->x - playerRect->w;
            platformRect2->x = platformRect2->x - playerRect->w;
            platformRect3->x = platformRect3->x - playerRect->w;
            movingPlatformRect->x = movingPlatformRect->x - playerRect->w;
			movingPlatformRect2->x = movingPlatformRect2->x - playerRect->w;
        }

		rightScrollCount = 0;
		leftScrollCount = 0;
    }
}

void Game::handleBoundaries() {
    auto& propertyManager = PropertyManager::getInstance();
	std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
	std::shared_ptr<RectProperty> rightBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(rightBoundaryID, "Rect"));
	std::shared_ptr<RectProperty> leftBoundaryRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(leftBoundaryID, "Rect"));
	std::shared_ptr<RectProperty> platformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
	std::shared_ptr<RectProperty> platformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID2, "Rect"));
	std::shared_ptr<RectProperty> platformRect3 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID3, "Rect"));
	std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
	//std::shared_ptr<RectProperty> spawnpointRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(spawnPointID, "Rect"));
	std::shared_ptr<RectProperty> movingPlatformRect2 = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID2, "Rect"));

    SDL_Rect playRect = { playerRect->x, playerRect->y, playerRect->w, playerRect->h };
	SDL_Rect rightRect = { rightBoundaryRect->x, rightBoundaryRect->y, rightBoundaryRect->w, rightBoundaryRect->h };
	SDL_Rect leftRect = { leftBoundaryRect->x, leftBoundaryRect->y, leftBoundaryRect->w, leftBoundaryRect->h };

	if (SDL_HasIntersection(&playRect, &rightRect)) {
		playerRect->x = rightBoundaryRect->x - playerRect->w;
		platformRect->x = platformRect->x - playerRect->w;
		platformRect2->x = platformRect2->x - playerRect->w;
		platformRect3->x = platformRect3->x - playerRect->w;
		movingPlatformRect->x = movingPlatformRect->x - playerRect->w;
		movingPlatformRect2->x = movingPlatformRect2->x - playerRect->w;
		//spawnpointRect->x = spawnpointRect->x - playerRect->w;
        rightScrollCount++;

	}

	if (SDL_HasIntersection(&playRect, &leftRect)) {
		playerRect->x = leftBoundaryRect->x + playerRect->w;
        platformRect->x = platformRect->x + playerRect->w;
        platformRect2->x = platformRect2->x + playerRect->w;
        platformRect3->x = platformRect3->x + playerRect->w;
        movingPlatformRect->x = movingPlatformRect->x + playerRect->w;
		movingPlatformRect2->x = movingPlatformRect2->x + playerRect->w;
		//spawnpointRect->x = spawnpointRect->x + playerRect->w;
        leftScrollCount++;

	}

}

// Update game object properties
void Game::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    playerRect->x += playerVel->vx;
    playerRect->y += playerVel->vy;

    // Simple gravity for the player
    if (playerRect->y < SCREEN_HEIGHT) {
        playerVel->vy += 1;  // Gravity effect
    }
    else {
        playerRect->y = SCREEN_HEIGHT;
        playerVel->vy = 0;
    }

    // Example: Update moving platform position
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID, "Velocity"));

    movingPlatformRect->x += movingPlatformVel->vx;
    if (movingPlatformRect->x <= 0 || movingPlatformRect->x >= SCREEN_WIDTH - 100) {  // Bounce within screen bounds
        movingPlatformVel->vx = -movingPlatformVel->vx;
    }

    // Update second moving platform (vertical movement)
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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render static platforms and moving platforms
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for static platforms
    renderPlatform(platformID);
    renderPlatform(platformID2);  // Render second platform
    renderPlatform(platformID3);  // Render third platform

    //SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);  // Cyan for moving platform
    renderPlatform(movingPlatformID);  // Render moving platform
	renderPlatform(movingPlatformID2);  // Render second moving platform

    // Render player character
    //SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow for player
    renderPlayer(playerID);

    SDL_RenderPresent(renderer);  // Present the updated screen
}

// Helper function to render platforms
void Game::renderPlatform(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(platformID, "Rect"));
    SDL_Rect platformRect = { rect->x, rect->y, rect->w, rect->h };  // Example size for platforms
	std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(platformID, "Render"));
	SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &platformRect);
}

// Helper function to render player
void Game::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> rect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    SDL_Rect playerRect = { rect->x, rect->y, rect->w, rect->h };  // Example size for player
	std::shared_ptr<RenderProperty> render = std::static_pointer_cast<RenderProperty>(propertyManager.getProperty(playerID, "Render"));
	SDL_SetRenderDrawColor(renderer, render->r, render->g, render->b, 255);
    SDL_RenderFillRect(renderer, &playerRect);
}