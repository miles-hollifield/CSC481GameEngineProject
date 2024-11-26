#pragma once

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>
#include "Timeline.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"
#include "InputEvent.h"
#include "CollisionEvent.h"
#include <SDL2/SDL_ttf.h>

// Constants for screen dimensions
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Constants for object dimensions
#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 20
#define ALIEN_WIDTH 30
#define ALIEN_HEIGHT 20
#define PROJECTILE_WIDTH 5
#define PROJECTILE_HEIGHT 10

// Forward declarations for properties
class RectProperty;
class VelocityProperty;

/**
 * @brief The Game2 class implements the core Space Invaders game logic.
 */
class Game2 {
public:
    /**
     * @brief Constructs a Game2 object with an SDL renderer and ZeroMQ sockets.
     * @param renderer SDL renderer for drawing the game.
     * @param reqSocket ZeroMQ request socket for sending player position data.
     * @param subSocket ZeroMQ subscriber socket for receiving updates.
     * @param eventReqSocket ZeroMQ socket for sending event data.
     */
    Game2(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket);

    /**
     * @brief Destructor for Game2.
     */
    ~Game2();

    /**
     * @brief Starts the main game loop.
     */
    void run();

private:
    // Initialization and setup
    /**
     * @brief Initializes game objects such as the player, aliens, and projectiles.
     */
    void initGameObjects();

    // Update functions
    /**
     * @brief Updates the game state, including object movement and collision checks.
     */
    void update();

    /**
     * @brief Updates the position of game objects.
     */
    void updateGameObjects();

    // Event handling
    /**
     * @brief Processes input and raises corresponding events.
     */
    void handleEvents();

    /**
     * @brief Handles a spawn event for creating objects like projectiles.
     * @param objectID ID of the object to spawn.
     */
    void handleSpawn(int objectID);

    /**
     * @brief Resolves collisions between game objects.
     * @param obj1ID ID of the first object in the collision.
     * @param obj2ID ID of the second object in the collision.
     */
    void resolveCollision(int obj1ID, int obj2ID);

    /**
     * @brief Handles a death event for objects like projectiles or aliens.
     * @param objectID ID of the object triggering the death event.
     */
    void handleDeath(int objectID);

    void resetGame();

    void renderLevelText();

    // Rendering functions
    /**
     * @brief Renders all game objects, including the player, aliens, and projectiles.
     */
    void render();

    void renderAlienProjectile(int alienProjID);

    /**
     * @brief Renders the player character.
     * @param playerID ID of the player to render.
     */
    void renderPlayer(int playerID);

    /**
     * @brief Renders an alien object.
     * @param alienID ID of the alien to render.
     */
    void renderAlien(int alienID);

    /**
     * @brief Renders a projectile object.
     * @param projectileID ID of the projectile to render.
     */
    void renderProjectile(int projectileID);

    // Networking functions
    /**
     * @brief Sends the player's position to the server.
     */
    void sendMovementUpdate();

    /**
     * @brief Receives updates from the server.
     */
    void receivePlayerPositions();

    // SDL and networking variables
    SDL_Renderer* renderer;           ///< SDL renderer for drawing the game.
    SDL_Event e;                      ///< SDL event object for input handling.
    zmq::socket_t& reqSocket;         ///< ZeroMQ request socket for sending player data.
    zmq::socket_t& subSocket;         ///< ZeroMQ subscriber socket for updates.
    zmq::socket_t& eventReqSocket;    ///< ZeroMQ event socket for raising events.

    // Game object management
    int playerID;                     ///< ID of the player object.
    std::vector<int> alienIDs;        ///< IDs of alien objects.
    std::vector<int> projectileIDs;   ///< IDs of projectile objects.

    // Game state
    bool quit;       // Controls whether the game exits
    bool gameOver;   // Tracks whether the game is over and needs a reset

    // Other member variables
    TTF_Font* font; // Font for rendering level text
    SDL_Texture* levelTexture; // Texture for the level text
    SDL_Rect levelRect; // Rectangle for the level text position

    int level = 1;

    // Timeline for managing events
    Timeline gameTimeline;            ///< Timeline for event timestamps.

    std::vector<int> alienProjectileIDs; // List to track alien projectiles

};
