#include "game2.h"
#include "PropertyManager.h"
#include "SDL2/SDL.h"
#include <iostream>
#include <memory>

// Constructor
Game2::Game2(SDL_Renderer* renderer)
    : renderer(renderer), quit(false)
{
    initGameObjects();
}

// Destructor
Game2::~Game2() {}

// Initialize game objects (characters, platforms, etc.)
void Game2::initGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Create player object with position and velocity properties
    playerID = propertyManager.createObject();
    propertyManager.addProperty(playerID, "Position", std::make_shared<PositionProperty>(100, 400));
    propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create static platform objects
    platformID = propertyManager.createObject();
    propertyManager.addProperty(platformID, "Position", std::make_shared<PositionProperty>(50, 300));

    int platformID2 = propertyManager.createObject();
    propertyManager.addProperty(platformID2, "Position", std::make_shared<PositionProperty>(250, 350));

    int platformID3 = propertyManager.createObject();
    propertyManager.addProperty(platformID3, "Position", std::make_shared<PositionProperty>(450, 400));

    // Create moving platform
    int movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Position", std::make_shared<PositionProperty>(150, 250));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally

    // Create death zone (hidden object)
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Position", std::make_shared<PositionProperty>(0, SCREEN_HEIGHT - 50));  // Just above the bottom of the screen
}

// Main game loop
void Game2::run() {
    while (!quit) {
        handleEvents();
        updateGameObjects();
        render();
        SDL_Delay(16);  // Cap frame rate
    }
}

// Handle player input and SDL events
void Game2::handleEvents() {
    SDL_Event e;
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<PositionProperty> playerPos = std::static_pointer_cast<PositionProperty>(propertyManager.getProperty(playerID, "Position"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        // Keyboard input to control player movement
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_LEFT]) {
            playerVel->vx = -5;
        }
        else if (keystates[SDL_SCANCODE_RIGHT]) {
            playerVel->vx = 5;
        }
        else {
            playerVel->vx = 0;
        }

        if (keystates[SDL_SCANCODE_UP] && playerPos->y == 400) {  // Allow jumping only when on the ground
            playerVel->vy = -10;  // Jump velocity
        }
    }
}

// Update game object properties
void Game2::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<PositionProperty> playerPos = std::static_pointer_cast<PositionProperty>(propertyManager.getProperty(playerID, "Position"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    playerPos->x += playerVel->vx;
    playerPos->y += playerVel->vy;

    // Simple gravity for the player
    if (playerPos->y < 400) {  // 400 is the ground level for simplicity
        playerVel->vy += 1;  // Gravity effect
    }
    else {
        playerPos->y = 400;
        playerVel->vy = 0;
    }

    // Example: Update moving platform position
    std::shared_ptr<PositionProperty> movingPlatformPos = std::static_pointer_cast<PositionProperty>(propertyManager.getProperty(platformID, "Position"));
    std::shared_ptr<VelocityProperty> movingPlatformVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(platformID, "Velocity"));

    movingPlatformPos->x += movingPlatformVel->vx;
    if (movingPlatformPos->x <= 0 || movingPlatformPos->x >= SCREEN_WIDTH - 100) {  // Bounce within screen bounds
        movingPlatformVel->vx = -movingPlatformVel->vx;
    }
}

// Render game objects to the screen
void Game2::render() {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Render static platforms and moving platforms
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red for static platforms
    renderPlatform(platformID);
    renderPlatform(2);  // Render second platform
    renderPlatform(3);  // Render third platform

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);  // Cyan for moving platform
    renderPlatform(4);  // Render moving platform

    // Render player character
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);  // Yellow for player
    renderPlayer(playerID);

    SDL_RenderPresent(renderer);  // Present the updated screen
}

// Helper function to render platforms
void Game2::renderPlatform(int platformID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<PositionProperty> pos = std::static_pointer_cast<PositionProperty>(propertyManager.getProperty(platformID, "Position"));
    SDL_Rect platformRect = { pos->x, pos->y, 100, 20 };  // Example size for platforms
    SDL_RenderFillRect(renderer, &platformRect);
}

// Helper function to render player
void Game2::renderPlayer(int playerID) {
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<PositionProperty> pos = std::static_pointer_cast<PositionProperty>(propertyManager.getProperty(playerID, "Position"));
    SDL_Rect playerRect = { pos->x, pos->y, 50, 50 };  // Example size for player
    SDL_RenderFillRect(renderer, &playerRect);
}
