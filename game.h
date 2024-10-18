#ifndef GAME2_H
#define GAME2_H

#include "SDL2/SDL.h"
#include "PropertyManager.h"
#include <unordered_map>
#include <memory>
#include "defs.h"

// Class representing the Game2 (new game with property-based model)
class Game {
public:
    // Constructor and Destructor
    Game(SDL_Renderer* renderer);  // Constructor with SDL renderer
    ~Game();  // Destructor

    // Method to run the main game loop
    void run();

private:
    SDL_Renderer* renderer;  // SDL renderer for rendering game objects
    bool quit;  // Flag to control the game loop

    int playerID;  // ID for the player object
    int platformID;  // ID for a static platform
    int platformID2;
    int platformID3;
	int movingPlatformID;  // ID for a moving platform
    int spawnPointID;
    int deathZoneID;  // ID for a death zone object

	int rightBoundaryID;
	int leftBoundaryID;
    int leftScrollCount;
	int rightScrollCount;

    // Initialize the game objects such as player, platforms, etc.
    void initGameObjects();

    // Handle player input and SDL events
    void handleEvents();

    void handleCollision(int platformID);

    void handleDeathzone();

	void handleBoundaries();

    // Update game object properties based on logic (e.g., movement, collisions)
    void updateGameObjects();

    // Render game objects on the screen
    void render();

    // Helper functions to render specific objects
    void renderPlayer(int playerID);  // Render player object
    void renderPlatform(int platformID);  // Render platform object
};

#endif // GAME2_H