#pragma once

#include <SDL2/SDL.h>
#include <zmq.hpp>
#include <deque>
#include <unordered_map>
#include <memory>
#include "Timeline.h"
#include "PropertyManager.h"
#include "ThreadManager.h"
#include "EventManager.h"
#include "DeathEvent.h"
#include "SpawnEvent.h"
#include <SDL2/SDL_ttf.h>

// Constants for screen dimensions
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Constants for grid and snake
#define GRID_SIZE 40  // Size of each grid cell
#define INITIAL_SPEED 0.5f  // Initial game speed
#define FOOD_SCORE 10  // Points per food
#define INITIAL_SNAKE_LENGTH 6

// Define GameType enumeration
enum GameType {
    PLATFORMER = 1,
    SNAKE,
    SPACE_INVADERS
};


/**
 * @brief The Game3 class implements the core Snake game logic with client-server networking.
 */
class Game3 {
public:
    /**
     * @brief Constructs a Game3 object with an SDL renderer and ZeroMQ sockets.
     * @param renderer SDL renderer for drawing the game.
     * @param reqSocket ZeroMQ request socket for sending player position data.
     * @param subSocket ZeroMQ subscriber socket for receiving updates.
     * @param eventReqSocket ZeroMQ socket for sending event data.
     */
    Game3(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket, zmq::socket_t& eventReqSocket);

    /**
     * @brief Destructor for Game3.
     */
    ~Game3();

    /**
     * @brief Starts the main game loop.
     */
    void run();

private:
    // Initialization and setup
    /**
     * @brief Initializes the game objects such as the snake and food.
     */
    void initGameObjects();

    /**
     * @brief Places food randomly on the grid.
     */
    void placeFood();

    /**
     * @brief Checks for collisions with the snake.
     * @param position The position to check for collision.
     * @return True if there's a collision, otherwise false.
     */
    bool checkCollision(const SDL_Point& position);

    // Update functions
    /**
     * @brief Updates the game state, including snake movement and collision checks.
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
     * @brief Handles a food event when food is consumed by the snake.
     * @param objectID ID of the food object.
     */
    void handleFoodEvent(int objectID);

    /**
     * @brief Handles a game-over event when the snake collides with itself or the walls.
     * @param objectID ID of the object triggering the event.
     */
    void handleGameOverEvent(int objectID);

    // Networking functions
    /**
     * @brief Sends the player's snake data to the server.
     */
    void sendPlayerUpdate();

    /**
     * @brief Receives updates from the server, including other players' snakes.
     */
    void receiveServerUpdates();

    // Rendering functions
    /**
     * @brief Renders all game objects, including the snake and food.
     */
    void render();

    /**
     * @brief Renders the score text on the screen.
     */
    void renderScoreText();

    // SDL and networking variables
    SDL_Renderer* renderer;           ///< SDL renderer for drawing the game.
    SDL_Event e;                      ///< SDL event object for input handling.
    zmq::socket_t& reqSocket;         ///< ZeroMQ request socket for sending player data.
    zmq::socket_t& subSocket;         ///< ZeroMQ subscriber socket for updates.
    zmq::socket_t& eventReqSocket;    ///< ZeroMQ event socket for raising events.

    // Game object management
    int snakeID;                      ///< ID of the snake's head.
    std::deque<SDL_Point> snakeBody;  ///< The snake body represented as a deque of grid positions.
    SDL_Point food;                   ///< Position of the food on the grid.
    SDL_Point direction;              ///< Current movement direction of the snake.
    std::unordered_map<int, std::deque<SDL_Point>> otherSnakes; ///< Other players' snakes.

    // Game state
    bool quit;                        ///< Tracks whether the game should exit.
    bool gameOver;                    ///< Tracks whether the game is over and needs a reset.
    int score;                        ///< Current score.

    // Other member variables
    TTF_Font* font;                   ///< Font for rendering score text.
    SDL_Texture* scoreTexture; // Texture for the score text
    SDL_Texture* speedTexture; // Texture for the speed text
    SDL_Rect scoreRect;        // Rectangle for the score text position and size
    SDL_Rect speedRect;        // Rectangle for the speed text position and size

    // Timeline for managing events
    Timeline gameTimeline;            ///< Timeline for event timestamps.
};
