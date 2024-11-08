#pragma once

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>
#include "Timeline.h"       // For timeline functionality (pausing/unpausing, time scaling)
#include "PropertyManager.h" // For property-based game objects
#include "ThreadManager.h"   // For multithreading platform updates
#include "EventManager.h"    // Event management system
#include "DeathEvent.h"      // Specific event types
#include "SpawnEvent.h"
#include "InputEvent.h"

// Define the structure to represent the position of a player
struct PlayerPosition {
    int x, y;  // X and Y coordinates of the player
};

// Constants for screen dimensions
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Forward declarations for properties used in the game (RectProperty, VelocityProperty)
class RectProperty;
class VelocityProperty;

/**
 * @brief The Game class contains all core game logic, manages the game state, and renders the game.
 */
class Game {
public:
    /**
     * @brief Constructs a Game object with an SDL renderer and ZeroMQ sockets.
     * @param renderer SDL renderer for drawing the game.
     * @param reqSocket ZeroMQ request socket for sending player position data to the server.
     * @param subSocket ZeroMQ subscriber socket for receiving updates from the server.
     * @param eventReqSocket ZeroMQ socket for sending event data to the server.
     */
    Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket);

    /**
     * @brief Destructor to clean up resources when the game is destroyed.
     */
    ~Game();

    /**
     * @brief Main game loop that handles updates, event processing, and rendering.
     */
    void run();

private:
    // Initialization and setup
    /**
     * @brief Initializes all game objects (e.g., players, platforms).
     */
    void initGameObjects();

    // Game object update functions
    /**
     * @brief Updates the overall game state, including player movement and collision checks.
     */
    void update();

    /**
     * @brief Updates the positions of all game objects.
     */
    void updateGameObjects();

    /**
     * @brief Adjusts the camera position based on the player's movement.
     */
    void updateCamera();

    // Collision handling functions
    /**
     * @brief Detects and handles collisions with platforms, boundaries, and death zones.
     */
    void checkCollisions();

    /**
     * @brief Handles collisions between the player and platforms.
     * @param platformID ID of the platform the player collides with.
     */
    void handleCollision(int platformID);

    /**
     * @brief Handles the event when the player falls into the death zone.
     */
    void handleDeathzone();

    /**
     * @brief Handles collisions with screen boundaries to prevent players from moving off-screen.
     */
    void handleBoundaries();

    // Event handling functions
    /**
     * @brief Handles a death event for a specified object.
     * @param objectID ID of the object triggering the death event.
     */
    void handleDeath(int objectID);

    /**
     * @brief Handles a spawn event for a specified object.
     * @param objectID ID of the object triggering the spawn event.
     */
    void handleSpawn(int objectID);

    /**
     * @brief Handles input events for a specified object.
     * @param objectID ID of the object receiving input.
     * @param inputAction Type of input action (e.g., MOVE_LEFT, JUMP).
     */
    void handleInput(int objectID, const InputAction& inputAction);

    /**
     * @brief Resolves collision between two specified objects.
     * @param obj1ID ID of the first object in the collision.
     * @param obj2ID ID of the second object in the collision.
     */
    void resolveCollision(int obj1ID, int obj2ID);

    // Rendering functions
    /**
     * @brief Renders all game objects (e.g., players, platforms) to the screen.
     */
    void render();

    /**
     * @brief Renders a specific platform based on its ID.
     * @param platformID ID of the platform to render.
     */
    void renderPlatform(int platformID);

    /**
     * @brief Renders the player character based on their ID.
     * @param playerID ID of the player to render.
     */
    void renderPlayer(int playerID);

    // Input handling and networking functions
    /**
     * @brief Processes input events (e.g., keyboard) and handles them accordingly.
     */
    void handleEvents();

    /**
     * @brief Sends the player's position update to the server.
     */
    void sendMovementUpdate();

    /**
     * @brief Receives the positions of all players from the server.
     */
    void receivePlayerPositions();

    /**
     * @brief Sends a spawn event to the server.
     * @param objectID ID of the object to spawn.
     * @param spawnX X-coordinate for spawning.
     * @param spawnY Y-coordinate for spawning.
     * @return SpawnEventData containing the spawn location.
     */
    SpawnEventData sendSpawnEvent(int objectID, int spawnX, int spawnY);

    // SDL-related variables
    SDL_Renderer* renderer;  // SDL renderer responsible for drawing game objects to the screen
    SDL_Event e;             // SDL event object used for handling input events

    // Networking-related variables
    zmq::socket_t& reqSocket;      // ZeroMQ request socket for player position data
    zmq::socket_t& subSocket;      // ZeroMQ subscriber socket for updates
    zmq::socket_t& eventReqSocket; // ZeroMQ request socket for event data

    // Game object and property IDs
    int clientId;                // Unique ID assigned to the player's character
    int playerID;                // ID of the player's object
    int platformID, platformID2, platformID3; ///< IDs for static platforms
    int movingPlatformID, movingPlatformID2; ///< IDs for moving platforms
    int deathZoneID;             // ID for the death zone
    int rightBoundaryID, leftBoundaryID; ///< IDs for screen boundaries
    int spawnPointID;            // ID for the player's spawn point

    // Player positions and rendering
    std::unordered_map<int, PlayerPosition> allPlayers; // Map storing positions of all players
    std::unordered_map<int, SDL_Rect> allRects;         // Map for rendering each player

    // Timeline and time management
    Timeline gameTimeline; // Manages pausing, unpausing, and time scaling
    std::chrono::steady_clock::time_point lastTime; // Last recorded time for frame delta calculations

    // Variables for moving platforms and screen boundaries
    std::mutex platformMutex; // Mutex for thread-safe updates to platform positions
    int rightScrollCount;     // Tracks camera scrolling to the right
    int leftScrollCount;      // Tracks camera scrolling to the left

    bool quit; ///< Flag indicating if the game loop should stop

    // Thread and event management
    ThreadManager threadManager; // Manages multithreading for platform movement
    EventManager eventManager;   // Manages game events

    // Camera variables
    int cameraX; // X-coordinate of the camera for side-scrolling
    int cameraY; // Y-coordinate of the camera for vertical scrolling
};
