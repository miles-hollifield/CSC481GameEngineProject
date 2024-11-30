#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>
#include <cmath>

// Constants for game configuration
#define SCREEN_WIDTH 1920 // Width of the game screen
#define SCREEN_HEIGHT 1080 // Height of the game screen
#define HEARTBEAT_INTERVAL_MS 10000 // Time interval to detect inactive clients
#define GRID_SIZE 20 // Grid size for game object positioning

// Enum for different game types
enum GameType {
    PLATFORMER = 1, // Represents a platformer game
    SNAKE,          // Represents the Snake game
    SPACE_INVADERS  // Represents the Space Invaders game
};

// Player position structure
struct PlayerPosition {
    int x, y; // x and y coordinates
};

// Structure to store player state
struct PlayerState {
    PlayerPosition pos; // Current position of the player
    int score; // Player's score for games that use scoring
    GameType gameType; // The game the player is playing
};

// Structure for spawn event data
struct SpawnEventData {
    int spawnX, spawnY; // Coordinates for spawn position
};

// Global data structures
std::unordered_map<int, PlayerState> players; // Maps client IDs to player states
std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeat; // Tracks client heartbeats
std::mutex playersMutex; // Mutex to ensure thread safety for shared data
int nextClientId = 0; // Counter for assigning unique client IDs

// Specific data for game types
std::unordered_map<int, SpawnEventData> snakeGames; // Stores Snake game data for each client
std::unordered_map<int, int> spaceInvaderGames; // Stores level state for Space Invaders

// Handles incoming client requests
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::none);
            if (received) {
                int clientId;
                PlayerPosition pos;

                // Parse the client ID and position from the message
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&pos, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(pos));

                std::lock_guard<std::mutex> lock(playersMutex);

                if (clientId == -1) { // New client connection
                    clientId = nextClientId++;
                    players[clientId] = { pos, 0, PLATFORMER }; // Default to PLATFORMER game
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();
                    std::cout << "New player connected: " << clientId << std::endl;

                    // Reply with the assigned client ID
                    zmq::message_t reply(sizeof(clientId));
                    memcpy(reply.data(), &clientId, sizeof(clientId));
                    repSocket.send(reply, zmq::send_flags::none);
                }
                else { // Existing client update
                    players[clientId].pos = pos;
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

// Broadcasts player positions to all platformer clients
void broadcastPositions(zmq::socket_t& pubSocket) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lock(playersMutex);

        if (!players.empty()) {
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerPosition)));
            char* buffer = static_cast<char*>(update.data());

            // Serialize player data into the message
            for (const auto& player : players) {
                if (player.second.gameType == PLATFORMER) {
                    memcpy(buffer, &player.first, sizeof(player.first));
                    buffer += sizeof(player.first);
                    memcpy(buffer, &player.second.pos, sizeof(player.second.pos));
                    buffer += sizeof(player.second.pos);
                }
            }

            try {
                pubSocket.send(update, zmq::send_flags::none);
            }
            catch (const zmq::error_t& e) {
                std::cerr << "Error broadcasting positions: " << e.what() << std::endl;
            }
        }
    }
}

// Handles game-specific events (e.g., respawns)
void handleEvents(zmq::socket_t& eventRepSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = eventRepSocket.recv(request, zmq::recv_flags::none);
            if (received) {
                int clientId;
                SpawnEventData spawnData;

                // Parse the client ID and spawn data from the message
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&spawnData, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(spawnData));

                std::lock_guard<std::mutex> lock(playersMutex);
                std::cout << "Spawn event for client: " << clientId << std::endl;

                // Adjust spawn point if the area is occupied
                for (const auto& player : players) {
                    if (std::abs(player.second.pos.x - spawnData.spawnX) <= 25 &&
                        std::abs(player.second.pos.y - spawnData.spawnY) <= 25) {
                        spawnData.spawnX += 60; // Adjust the spawn X coordinate
                    }
                }

                // Send adjusted spawn data back to the client
                zmq::message_t reply(sizeof(clientId) + sizeof(spawnData));
                memcpy(reply.data(), &clientId, sizeof(clientId));
                memcpy(static_cast<char*>(reply.data()) + sizeof(clientId), &spawnData, sizeof(spawnData));
                eventRepSocket.send(reply, zmq::send_flags::none);
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "Error receiving event: " << e.what() << std::endl;
        }
    }
}

// Checks for disconnected clients based on heartbeat intervals
void checkForTimeouts() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(playersMutex);

        for (auto it = lastHeartbeat.begin(); it != lastHeartbeat.end();) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count() > HEARTBEAT_INTERVAL_MS) {
                int clientId = it->first;
                std::cout << "Client " << clientId << " disconnected." << std::endl;

                // Remove disconnected client from all data structures
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

// Main server function
int main() {
    zmq::context_t context(2);
    zmq::socket_t repSocket(context, zmq::socket_type::rep); // Socket for client requests
    zmq::socket_t pubSocket(context, zmq::socket_type::pub); // Socket for broadcasting updates
    zmq::socket_t eventRepSocket(context, zmq::socket_type::rep); // Socket for event handling

    // Bind sockets to ports
    repSocket.bind("tcp://*:5555");
    pubSocket.bind("tcp://*:5556");
    eventRepSocket.bind("tcp://*:5557");

    // Start threads for handling different server functions
    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));
    std::thread timeoutThread(checkForTimeouts);
    std::thread eventThread(handleEvents, std::ref(eventRepSocket));

    // Wait for threads to complete
    requestThread.join();
    broadcastThread.join();
    timeoutThread.join();
    eventThread.join();

    return 0;
}
