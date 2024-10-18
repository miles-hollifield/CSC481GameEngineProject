#ifndef GAME2_H
#define GAME2_H

#include "SDL2/SDL.h"
#include "PropertyManager.h"
#include <unordered_map>
#include <memory>
#include "defs.h"
#include <zmq.hpp>

// Class representing the Game2 (new game with property-based model)
class Game {
public:
    // Constructor and Destructor
    Game(SDL_Renderer* renderer);  // Constructor with SDL renderer
    ~Game();  // Destructor

    // Method to run the main game loop with ZeroMQ request socket
    void run(zmq::socket_t& reqSocket);

    // Update player position (used when receiving player positions from server)
    void updatePlayerPosition(int playerID, const PlayerPosition& pos);

    // Update platform position (used when receiving platform positions from server)
    void updatePlatformPosition(const PlayerPosition& pos);

    // Update vertical platform position (used when receiving vertical platform positions from server)
    void updateVerticalPlatformPosition(const PlayerPosition& pos);

private:
    SDL_Renderer* renderer;  // SDL renderer for rendering game objects
    bool quit;  // Flag to control the game loop

    int playerID;  // ID for the player object
    int platformID;  // ID for a static platform
    int platformID2;
    int platformID3;
    int movingPlatformID;  // ID for a moving platform
    int movingPlatformID2;  // ID for the second moving platform (vertical)
    int spawnPointID;
    int deathZoneID;  // ID for a death zone object

    int rightBoundaryID;
    int leftBoundaryID;
    int leftScrollCount;
    int rightScrollCount;

    // Initialize the game objects such as player, platforms, etc.
    void initGameObjects();

    // Handle player input, SDL events, and send updates to the server via ZeroMQ
    void handleEvents(zmq::socket_t& reqSocket);

    // Handle collisions with platforms or other objects
    void handleCollision(int platformID);

    // Handle death zone collision
    void handleDeathzone();

    // Handle boundary collisions
    void handleBoundaries();

    // Check for collisions and handle accordingly
    void checkCollisions();

    // Update game object properties based on logic (e.g., movement, collisions)
    void updateGameObjects();

    // Render game objects on the screen
    void render();

    // Helper functions to render specific objects
    void renderPlayer(int playerID);  // Render player object
    void renderPlatform(int platformID);  // Render platform object
};

#endif // GAME2_H
