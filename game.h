#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include "PhysicsEntity.h"
#include "defs.h"
#include "ThreadManager.h"
#include "Timeline.h"

// Define player position structure
struct PlayerPosition {
    int x, y;
};

// Class definition for Game
class Game {
public:
    /**
     * @brief Constructor for the Game class.
     *
     * Initializes the game with the given renderer, request socket, and subscription socket.
     *
     * @param renderer SDL_Renderer pointer used to render game objects.
     * @param reqSocket Reference to the ZeroMQ request socket for sending player updates.
     * @param subSocket Reference to the ZeroMQ subscription socket for receiving updates from the server.
     */
    Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket);

    /**
     * @brief Destructor for the Game class.
     */
    ~Game();

    /**
     * @brief Main game loop that runs until the quit flag is set.
     *
     * Handles input events, renders the game, and updates game state based on the timeline.
     */
    void run();  // Main game loop

    /**
     * @brief Represents the game's timeline, controlling the flow of time in the game.
     *
     * Handles tic rates, pausing/unpausing, and time scaling for the game.
     */
    Timeline gameTimeline;

private:
    SDL_Renderer* renderer;  // Pointer to the SDL renderer
    zmq::socket_t& reqSocket;  // Reference to the request socket for ZeroMQ
    zmq::socket_t& subSocket;  // Reference to the subscription socket for ZeroMQ
    SDL_Event e;  // SDL event handler
    bool quit;  // Flag to control the game loop
    int playerId;  // ID of the current player
    PlayerPosition myPlayer;  // Current player position
    std::unordered_map<int, PlayerPosition> allPlayers;  // Map storing positions of all players
    std::unordered_map<int, SDL_Rect> allRects;  // Map storing player rectangles for rendering

    // Platform and controllable shape variables
    SDL_Rect platformRect;  // Static platform
    SDL_Rect movingPlatformRect;  // Moving platform
    PhysicsEntity controllableRect;  // Controllable shape (player) with physics
    float movingPlatformVelocity;  // Velocity of the moving platform

    // Thread management and synchronization
    std::mutex platformMutex;  // Mutex to ensure thread-safe access to the platform
    ThreadManager threadManager;  // To manage threads
    std::chrono::time_point<std::chrono::steady_clock> lastTime;  // Last time for frame delta calculation

    /**
     * @brief Handles SDL input events and processes player movement based on the game timeline.
     *
     * @param frameDelta Time delta between frames, used to adjust player movement speed.
     */
    void handleEvents(float frameDelta);

    /**
     * @brief Sends the player's movement and position to the server.
     *
     * Packages the player's position and sends it through the ZeroMQ request socket.
     */
    void sendMovementUpdate();

    /**
     * @brief Receives updated player positions from the server.
     *
     * Updates the positions of all players, including the local player, and updates the moving platform.
     */
    void receivePlayerPositions();

    /**
     * @brief Updates the position of the moving platform.
     *
     * Moves the platform based on its velocity and reverses its direction when it reaches the screen edge.
     *
     * @param frameDelta Time delta between frames, used to adjust platform movement speed.
     */
    void updatePlatform(float frameDelta);

    /**
     * @brief Updates the game state, including physics calculations and player collision detection.
     *
     * @param frameDelta Time delta between frames, used to adjust game logic.
     */
    void update(float frameDelta);

    /**
     * @brief Renders the game objects, including the platforms, players, and other elements.
     *
     * Draws the static and moving platforms, controllable shape, and other players on the screen.
     *
     * @param renderer SDL_Renderer pointer used to render the game objects.
     * @param playerPositions Map containing positions of all players in the game.
     */
    void render(SDL_Renderer* renderer, const std::unordered_map<int, PlayerPosition>& playerPositions);
};

#endif // GAME_H
