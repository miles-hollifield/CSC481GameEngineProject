#pragma once

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "Timeline.h"  // For timeline functionality (pausing/unpausing, time scaling)
#include "PropertyManager.h"  // For property-based game objects
#include "ThreadManager.h"  // For multithreading platform updates

// Define the structure to represent the position of a player
struct PlayerPosition {
    int x, y;
};

// Constants for screen dimensions
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Forward declarations for properties used in the game (RectProperty, VelocityProperty)
class RectProperty;
class VelocityProperty;

// The Game class contains all the core game logic and manages the game state
class Game {
public:
    // Constructor and Destructor
    Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket);  // Initialize the game with the SDL renderer and ZeroMQ sockets
    ~Game();  // Clean up resources when the game is destroyed

    // Main game loop
    void run();  // Start the main game loop which handles updates and rendering

private:
    // Initialization and setup
    void initGameObjects();  // Set up all game objects (e.g., players, platforms) at the start of the game

    // Game object update functions
    void update();  // Update the overall game state (e.g., player movement, collision checks)
    void updateGameObjects();  // Update positions of all game objects (e.g., players, platforms)
    void updateCamera();  // Adjust the camera position based on player movement

    // Collision handling functions
    void checkCollisions();  // Detect and handle collisions with platforms, boundaries, and death zones
    void handleCollision(int platformID);  // Handle collisions between the player and platforms
    void handleDeathzone();  // Handle the event when the player falls into the death zone
    void handleBoundaries();  // Handle collisions with screen boundaries to prevent players from moving off-screen

    // Rendering functions
    void render();  // Render all game objects (players, platforms) to the screen
    void renderPlatform(int platformID);  // Render a specific platform based on its ID
    void renderPlayer(int playerID);  // Render the player character based on their ID

    // Input handling and networking functions
    void handleEvents();  // Process input events (keyboard, mouse) and handle them accordingly
    void sendMovementUpdate();  // Send the player's position update to the server
    void receivePlayerPositions();  // Receive the positions of all players from the server

    // SDL-related variables
    SDL_Renderer* renderer;  // The SDL renderer responsible for drawing game objects to the screen
    SDL_Event e;  // SDL event object used for handling input events (keyboard, mouse, etc.)

    // Networking-related variables
    zmq::socket_t& reqSocket;  // ZeroMQ request socket used to send player position data to the server
    zmq::socket_t& subSocket;  // ZeroMQ subscriber socket used to receive updates from the server

    // Game object and property IDs
    int clientId;  // The unique ID assigned to the player's character by the server
    int playerID;  // The ID of the player's object in the PropertyManager
    int platformID, platformID2, platformID3;  // IDs for static platforms
    int movingPlatformID, movingPlatformID2;  // IDs for moving platforms
    int deathZoneID;  // ID for the death zone (used to reset player position when they "die")
    int rightBoundaryID, leftBoundaryID;  // IDs for the right and left screen boundaries
    int spawnPointID;  // ID for the player's spawn point (where they respawn after falling into the death zone)

    // Player positions and rendering
    std::unordered_map<int, PlayerPosition> allPlayers;  // Map that stores the positions of all players (received from the server)
    std::unordered_map<int, SDL_Rect> allRects;  // Map to store the rectangles for rendering each player

    // Timeline and time management
    Timeline gameTimeline;  // Object that manages pausing, unpausing, and time scaling in the game
    std::chrono::steady_clock::time_point lastTime;  // Time point used for calculating frame delta (used in game updates)

    // Variables related to moving platforms and screen boundaries
    std::mutex platformMutex;  // Mutex used for thread-safe updates to platform positions
    int rightScrollCount;  // Counter to track how many times the camera has scrolled to the right
    int leftScrollCount;  // Counter to track how many times the camera has scrolled to the left

    // Quit flag
    bool quit;  // Boolean flag that indicates whether the game should stop running (quit the game loop)

    // Thread management
    ThreadManager threadManager;  // Object that manages multithreading, such as platform movement threads

    // Camera variables
    int cameraX;  // The X-coordinate of the camera (used to implement camera scrolling)
    int cameraY;  // The Y-coordinate of the camera (used to implement vertical camera scrolling)
};
