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
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), eventReqSocket(eventReqSocket), quit(false), gameTimeline(nullptr, 1.0f) {
    initGameObjects();
}

// Destructor
Game2::~Game2() {}

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
    propertyManager.addProperty(playerID, "Rect", std::make_shared<RectProperty>(SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - 60, PLAYER_WIDTH, PLAYER_HEIGHT));
    propertyManager.addProperty(playerID, "Render", std::make_shared<RenderProperty>(0, 255, 0)); // Green player
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create aliens in a grid
    int numColumns = 10;
    int numRows = 5;
    int totalAlienWidth = numColumns * (ALIEN_WIDTH + 10) - 10; // Total width of the alien grid (spacing included)
    int startX = (SCREEN_WIDTH - totalAlienWidth) / 2;          // Center horizontally
    int startY = 50;                                           // Starting Y position

    for (int i = 0; i < numRows; ++i) {  // Rows
        for (int j = 0; j < numColumns; ++j) {  // Columns
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
        handleEvents();        // Handle player input
        update();              // Update game objects
        render();              // Render objects to the screen
        SDL_Delay(16);         // Cap frame rate to ~60 FPS
    }
}

// Handle events
void Game2::handleEvents() {
    static bool isSpacePressed = false; // Tracks whether the spacebar is currently pressed

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }

    // Handle keyboard input
    const Uint8* keystates = SDL_GetKeyboardState(nullptr);
    auto& propertyManager = PropertyManager::getInstance();
    auto playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    if (keystates[SDL_SCANCODE_LEFT]) {
        playerVel->vx = -5; // Move left
    }
    else if (keystates[SDL_SCANCODE_RIGHT]) {
        playerVel->vx = 5; // Move right
    }
    else {
        playerVel->vx = 0; // Stop movement
    }

    // Handle spacebar for firing
    if (keystates[SDL_SCANCODE_SPACE]) {
        if (!isSpacePressed) {
            // Spacebar was just pressed, fire a projectile
            int projectileID = propertyManager.createObject();
            auto playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
            propertyManager.addProperty(projectileID, "Rect", std::make_shared<RectProperty>(
                playerRect->x + PLAYER_WIDTH / 2 - PROJECTILE_WIDTH / 2,
                playerRect->y,
                PROJECTILE_WIDTH,
                PROJECTILE_HEIGHT));
            propertyManager.addProperty(projectileID, "Render", std::make_shared<RenderProperty>(255, 255, 255)); // White projectile
            propertyManager.addProperty(projectileID, "Velocity", std::make_shared<VelocityProperty>(0, -10)); // Move up
            projectileIDs.push_back(projectileID);

            isSpacePressed = true; // Mark spacebar as pressed
        }
    }
    else {
        // Spacebar is not pressed
        isSpacePressed = false;
    }
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
    if (alienMoveTimer >= 30) { // Move aliens every 30 frames
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

    if (alienShootTimer >= 100) { // Aliens shoot every 100 frames
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
            EventManager::getInstance().raiseEvent(std::make_shared<DeathEvent>(playerID, &gameTimeline));
            propertyManager.destroyObject(*it);
            it = alienProjectileIDs.erase(it);
        }
        else if (projRect->y > SCREEN_HEIGHT) { // Remove projectile if off-screen
            propertyManager.destroyObject(*it);
            it = alienProjectileIDs.erase(it);
        }
        else {
            ++it;
        }
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
    PropertyManager::getInstance().destroyObject(objectID);
}