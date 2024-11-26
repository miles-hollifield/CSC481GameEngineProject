#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <deque>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define GRID_SIZE 20
#define HEARTBEAT_INTERVAL_MS 10000

// Game identifiers
enum GameType {
    PLATFORMER = 1,
    SNAKE,
    SPACE_INVADERS
};

// Common player state
struct PlayerState {
    int x, y;         // Position
    int score;        // Score
    GameType gameType; // Game type (Platformer, Snake, or Space Invaders)
};

// Snake-specific data
struct SnakeGameState {
    std::deque<std::pair<int, int>> body; // Snake body (list of grid cells)
    int directionX, directionY;           // Movement direction
};

// Space Invaders-specific data
struct SpaceInvadersGameState {
    int level;   // Current level of the player
    bool alive;  // Player's alive status
};

// Global data structures
std::unordered_map<int, PlayerState> players;
std::unordered_map<int, SnakeGameState> snakeGames;
std::unordered_map<int, SpaceInvadersGameState> spaceInvaderGames;
std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeat;
std::mutex playersMutex;
int nextClientId = 0;

// Function to handle client requests
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::none);

            if (received) {
                // Extract client ID, game type, and position
                int clientId;
                GameType gameType;
                PlayerState playerState;
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&gameType, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(gameType));
                memcpy(&playerState, static_cast<char*>(request.data()) + sizeof(clientId) + sizeof(gameType), sizeof(playerState));

                // Lock mutex for thread safety
                std::lock_guard<std::mutex> lock(playersMutex);

                if (clientId == -1) {
                    // New client connection
                    clientId = nextClientId++;
                    playerState.gameType = gameType;
                    players[clientId] = playerState;
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();

                    if (gameType == SNAKE) {
                        // Initialize Snake game state
                        SnakeGameState snakeState;
                        snakeState.body.push_back({ playerState.x / GRID_SIZE, playerState.y / GRID_SIZE });
                        snakeState.directionX = 1;
                        snakeState.directionY = 0;
                        snakeGames[clientId] = snakeState;
                    }
                    else if (gameType == SPACE_INVADERS) {
                        // Initialize Space Invaders game state
                        SpaceInvadersGameState invadersState = { 1, true }; // Level 1, player alive
                        spaceInvaderGames[clientId] = invadersState;
                    }

                    std::cout << "New client connected: " << clientId << " (Game: " << gameType << ")" << std::endl;

                    zmq::message_t reply(sizeof(clientId));
                    memcpy(reply.data(), &clientId, sizeof(clientId));
                    repSocket.send(reply, zmq::send_flags::none);
                }
                else {
                    // Existing client
                    players[clientId] = playerState;
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();
                    zmq::message_t reply("OK", 2);
                    repSocket.send(reply, zmq::send_flags::none);
                }
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "Error receiving message: " << e.what() << std::endl;
        }
    }
}

// Broadcast player states and game-specific data
void broadcastGameState(zmq::socket_t& pubSocket) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::lock_guard<std::mutex> lock(playersMutex);
        if (!players.empty()) {
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerState)));
            char* buffer = static_cast<char*>(update.data());

            for (const auto& player : players) {
                int id = player.first;
                PlayerState state = player.second;

                memcpy(buffer, &id, sizeof(id));
                buffer += sizeof(id);
                memcpy(buffer, &state, sizeof(state));
                buffer += sizeof(state);
            }

            pubSocket.send(update, zmq::send_flags::none);
        }
    }
}

// Check for disconnected clients
void checkForTimeouts() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(playersMutex);

        for (auto it = lastHeartbeat.begin(); it != lastHeartbeat.end();) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count() > HEARTBEAT_INTERVAL_MS) {
                int clientId = it->first;
                std::cout << "Client " << clientId << " timed out." << std::endl;

                players.erase(clientId);
                snakeGames.erase(clientId);
                spaceInvaderGames.erase(clientId);
                it = lastHeartbeat.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

// Handle game-specific events
void handleEvents(zmq::socket_t& eventRepSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = eventRepSocket.recv(request, zmq::recv_flags::none);

            if (received) {
                int clientId;
                char eventData[256]; // Generic event data
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(eventData, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(eventData));

                std::lock_guard<std::mutex> lock(playersMutex);
                if (players.find(clientId) != players.end()) {
                    PlayerState& player = players[clientId];
                    std::cout << "Event received from client " << clientId << ": " << eventData << std::endl;

                    // Game-specific event handling (expand as needed)
                    if (player.gameType == SNAKE) {
                        // Handle Snake game events
                    }
                    else if (player.gameType == SPACE_INVADERS) {
                        // Handle Space Invaders game events
                    }
                    else if (player.gameType == PLATFORMER) {
                        // Handle Platformer game events
                    }

                    zmq::message_t reply("Event OK", 8);
                    eventRepSocket.send(reply, zmq::send_flags::none);
                }
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "Error receiving event: " << e.what() << std::endl;
        }
    }
}

int main() {
    zmq::context_t context(2);
    zmq::socket_t repSocket(context, zmq::socket_type::rep);
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);
    zmq::socket_t eventRepSocket(context, zmq::socket_type::rep);

    repSocket.bind("tcp://*:5555");
    pubSocket.bind("tcp://*:5556");
    eventRepSocket.bind("tcp://*:5557");

    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastGameState, std::ref(pubSocket));
    std::thread timeoutThread(checkForTimeouts);
    std::thread eventThread(handleEvents, std::ref(eventRepSocket));

    requestThread.join();
    broadcastThread.join();
    timeoutThread.join();
    eventThread.join();

    return 0;
}
