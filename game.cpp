#include "game.h"
#include "PropertyManager.h"
#include "SDL2/SDL.h"
#include <iostream>
#include <memory>

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
	propertyManager.addProperty(playerID, "Physics", std::make_shared<PhysicsProperty>(0.0f));
	propertyManager.addProperty(playerID, "Collision", std::make_shared<CollisionProperty>(true));
	propertyManager.addProperty(playerID, "Velocity", std::make_shared<VelocityProperty>(0, 0));

    // Create static platform objects
    platformID = propertyManager.createObject();
    propertyManager.addProperty(platformID, "Rect", std::make_shared<RectProperty>(50, 300, 200, 50));
	propertyManager.addProperty(platformID, "Render", std::make_shared<RenderProperty>(0, 255, 0));
	propertyManager.addProperty(platformID, "Collision", std::make_shared<CollisionProperty>(true));

    platformID2 = propertyManager.createObject();
    propertyManager.addProperty(platformID2, "Rect", std::make_shared<RectProperty>(250, 350, 200, 50));
	propertyManager.addProperty(platformID2, "Render", std::make_shared<RenderProperty>(0, 0, 255));
	propertyManager.addProperty(platformID2, "Collision", std::make_shared<CollisionProperty>(true));

    platformID3 = propertyManager.createObject();
    propertyManager.addProperty(platformID3, "Rect", std::make_shared<RectProperty>(450, 400, 200, 50));
	propertyManager.addProperty(platformID3, "Render", std::make_shared<RenderProperty>(0, 255, 255));
	propertyManager.addProperty(platformID3, "Collision", std::make_shared<CollisionProperty>(true));

    // Create moving platform
    movingPlatformID = propertyManager.createObject();
    propertyManager.addProperty(movingPlatformID, "Rect", std::make_shared<RectProperty>(150, 250, 200, 50));
	propertyManager.addProperty(movingPlatformID, "Render", std::make_shared<RenderProperty>(255, 255, 0));
	propertyManager.addProperty(movingPlatformID, "Collision", std::make_shared<CollisionProperty>(true));
    propertyManager.addProperty(movingPlatformID, "Velocity", std::make_shared<VelocityProperty>(2, 0));  // Moving horizontally

    // Create death zone (hidden object)
    deathZoneID = propertyManager.createObject();
    propertyManager.addProperty(deathZoneID, "Rect", std::make_shared<RectProperty>(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50));  // Just above the bottom of the screen
	propertyManager.addProperty(deathZoneID, "Collision", std::make_shared<CollisionProperty>(true));  // Enable collision detection

}

// Main game loop
void Game::run() {
    while (!quit) {
        handleEvents();
        updateGameObjects();
        render();
        SDL_Delay(16);  // Cap frame rate
    }
}

// Handle player input and SDL events
void Game::handleEvents() {
    SDL_Event e;
    auto& propertyManager = PropertyManager::getInstance();
    std::shared_ptr<RectProperty> playerPos = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
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
void Game::updateGameObjects() {
    auto& propertyManager = PropertyManager::getInstance();

    // Update player position based on velocity
    std::shared_ptr<RectProperty> playerRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(playerID, "Rect"));
    std::shared_ptr<VelocityProperty> playerVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(playerID, "Velocity"));

    playerRect->x += playerVel->vx;
    playerRect->y += playerVel->vy;

    // Simple gravity for the player
    if (playerRect->y < 400) {  // 400 is the ground level for simplicity
        playerVel->vy += 1;  // Gravity effect
    }
    else {
        playerRect->y = 400;
        playerVel->vy = 0;
    }

    // Example: Update moving platform position
    std::shared_ptr<RectProperty> movingPlatformRect = std::static_pointer_cast<RectProperty>(propertyManager.getProperty(movingPlatformID, "Rect"));
    std::shared_ptr<VelocityProperty> movingPlatformVel = std::static_pointer_cast<VelocityProperty>(propertyManager.getProperty(movingPlatformID, "Velocity"));

    movingPlatformRect->x += movingPlatformVel->vx;
    if (movingPlatformRect->x <= 0 || movingPlatformRect->x >= SCREEN_WIDTH - 100) {  // Bounce within screen bounds
        movingPlatformVel->vx = -movingPlatformVel->vx;
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
    renderPlatform(2);  // Render second platform
    renderPlatform(3);  // Render third platform

    //SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);  // Cyan for moving platform
    renderPlatform(4);  // Render moving platform

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