#pragma once

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "Timeline.h"  // For timeline functionality (pausing/unpausing, time scaling)
#include "PropertyManager.h"  // For property-based game objects
#include "ThreadManager.h"  // For multithreading platform updates

// Define player position structure
struct PlayerPosition {
    int x, y;
};

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Forward declarations
class RectProperty;
class VelocityProperty;

// The Game class represents the main game logic and state
class Game {
public:
    // Constructor and Destructor
    Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket);
    ~Game();

    // Main game loop
    void run();

private:
    // Initialization and setup
    void initGameObjects();  // Initializes the game objects (players, platforms, etc.)

    // Game object update functions
    void update(float frameDelta);  // Update game state
    void updateGameObjects(float frameDelta);  // Updates positions of players, platforms, etc.

    // Collision handling functions
    void checkCollisions();  // Check for collisions with platforms, boundaries, and death zones
    void handleCollision(int platformID);  // Handle player-platform collisions
    void handleDeathzone();  // Handle player entering death zone
    void handleBoundaries();  // Handle player colliding with screen boundaries

    // Rendering functions
    void render();  // Renders all game objects on the screen
    void renderPlatform(int platformID);  // Renders a platform by its ID
    void renderPlayer(int playerID);  // Renders a player by its ID

    // Input handling and networking functions
    void handleEvents(float frameDelta);  // Handles SDL events and player input
    void sendMovementUpdate();  // Sends the current player's position to the server
    void receivePlayerPositions();  // Receives all players' positions from the server

    // SDL variables
    SDL_Renderer* renderer;  // The SDL renderer used to draw on the screen
    SDL_Event e;  // SDL event for handling input

    // Networking variables
    zmq::socket_t& reqSocket;  // ZeroMQ request socket (REQ) to send player position to the server
    zmq::socket_t& subSocket;  // ZeroMQ subscriber socket (SUB) to receive updates from the server

    // Game objects and properties
    int playerId;  // The ID assigned to this player's character by the server
    int playerID;  // Player object ID in the PropertyManager
    int platformID, platformID2, platformID3;  // IDs for static platforms
    int movingPlatformID, movingPlatformID2;  // IDs for moving platforms
    int deathZoneID;  // ID for the death zone
    int rightBoundaryID, leftBoundaryID;  // IDs for screen boundaries
    int spawnPointID;  // ID for the player's spawn point

    std::unordered_map<int, PlayerPosition> allPlayers;  // Map of all players' positions (received from the server)
    std::unordered_map<int, SDL_Rect> allRects;  // Map to store player rectangles for rendering

    // Timeline and timing
    Timeline gameTimeline;  // Used for managing game pause/unpause and time scaling
    std::chrono::steady_clock::time_point lastTime;  // Last time for frame delta calculation

    // Moving platform and boundary variables
    std::mutex platformMutex;  // Mutex to ensure thread safety when updating platform positions
    int rightScrollCount;  // Tracks right scrolling count for boundary movement
    int leftScrollCount;  // Tracks left scrolling count for boundary movement

    // Quit flag
    bool quit;  // Flag to indicate whether the game should quit

    // Threads
    ThreadManager threadManager;  // For managing threads (e.g., platform movement)
};
