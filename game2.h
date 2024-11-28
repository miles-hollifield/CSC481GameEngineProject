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
#define SCREEN_WIDTH 1920  // The width of the game window
#define SCREEN_HEIGHT 1080 // The height of the game window

// Constants for object dimensions
#define PLAYER_WIDTH 80         // Width of the player object
#define PLAYER_HEIGHT 40        // Height of the player object
#define ALIEN_WIDTH 60          // Width of an alien object
#define ALIEN_HEIGHT 40         // Height of an alien object
#define PROJECTILE_WIDTH 10     // Width of a projectile
#define PROJECTILE_HEIGHT 20    // Height of a projectile

// Forward declarations for properties
class RectProperty;
class VelocityProperty;

// Define the structure to represent the position of a player
struct PlayerPosition {
    int x, y;  // X and Y coordinates of the player
};

// The Game2 class implements the core Space Invaders game logic with client-server networking
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
    /**
     * @brief Initializes game objects such as the player, aliens, and projectiles.
     */
    void initGameObjects();

    /**
     * @brief Updates the game state, including object movement and collision checks.
     */
    void update();

    /**
     * @brief Updates the position of game objects.
     */
    void updateGameObjects();

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
     * @brief Handles a death event for objects like projectiles or aliens.
     * @param objectID ID of the object triggering the death event.
     */
    void handleDeath(int objectID);

    /**
     * @brief Resets the game state, including reinitializing all objects and resetting the timeline.
     */
    void resetGame();

    /**
     * @brief Sends the player's position to the server.
     */
    void sendPlayerUpdate();

    /**
     * @brief Receives updates from the server, including other players' positions.
     */
    void receiveServerUpdates();

    /**
     * @brief Renders all game objects, including the player, aliens, and projectiles.
     */
    void render();

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

    /**
     * @brief Renders an alien projectile object.
     * @param alienProjID ID of the alien projectile to render.
     */
    void renderAlienProjectile(int alienProjID);

    /**
     * @brief Renders the current level text on the screen.
     */
    void renderLevelText();

    /**
     * @brief Fires a projectile from the player's position.
     */
    void fireProjectile();

    SDL_Renderer* renderer;           // SDL renderer for drawing the game
    SDL_Event e;                      // SDL event object for input handling
    zmq::socket_t& reqSocket;         // ZeroMQ request socket for sending player data
    zmq::socket_t& subSocket;         // ZeroMQ subscriber socket for updates
    zmq::socket_t& eventReqSocket;    // ZeroMQ event socket for raising events

    int playerID;                     // ID of the player object
    std::vector<int> alienIDs;        // IDs of alien objects
    std::vector<int> projectileIDs;   // IDs of projectile objects
    std::vector<int> alienProjectileIDs; // IDs of alien projectile objects
    std::unordered_map<int, PlayerPosition> allPlayers; // Map of all players' positions

    bool quit;                        // Whether the game is running
    bool gameOver;                    // Whether the game is over
    int clientId;                     // Unique client ID assigned by the server
    int level = 1;                    // Current game level

    TTF_Font* font;                   // Font for rendering text
    SDL_Texture* levelTexture;        // Texture for the level text
    SDL_Texture* speedTexture;        // Texture for the speed text
    SDL_Rect levelRect;               // Rectangle for the level text position and size
    SDL_Rect speedRect;               // Rectangle for the speed text position and size

    Timeline gameTimeline;            // Timeline for managing game speed
};
