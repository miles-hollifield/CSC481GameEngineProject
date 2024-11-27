#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>
#include <cmath>

// Constants
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define HEARTBEAT_INTERVAL_MS 10000
#define GRID_SIZE 20

// Enum for game types
enum GameType {
    PLATFORMER = 1,
    SNAKE,
    SPACE_INVADERS
};

// Structures for player data
struct PlayerPosition {
    int x, y;
};

struct PlayerState {
    PlayerPosition pos;  // Position of the player
    int score;           // Score for Snake or Space Invaders
    GameType gameType;   // The game type the player is playing
};

// Spawn event data structure
struct SpawnEventData {
    int spawnX, spawnY;
};

// Global state
std::unordered_map<int, PlayerState> players;
std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeat;
std::mutex playersMutex;
int nextClientId = 0;

// Snake and Space Invaders specific data
std::unordered_map<int, SpawnEventData> snakeGames;  // Snake-specific state
std::unordered_map<int, int> spaceInvaderGames;      // Level state for Space Invaders

// Function to handle incoming requests from clients
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::none);

            if (received) {
                int clientId;
                PlayerPosition pos;
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&pos, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(pos));

                std::lock_guard<std::mutex> lock(playersMutex);

                if (clientId == -1) {  // New client
                    clientId = nextClientId++;
                    players[clientId] = { pos, 0, PLATFORMER };  // Default game type is PLATFORMER
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();
                    std::cout << "New player connected: " << clientId << std::endl;

                    zmq::message_t reply(sizeof(clientId));
                    memcpy(reply.data(), &clientId, sizeof(clientId));
                    repSocket.send(reply, zmq::send_flags::none);
                }
                else {  // Existing client
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

// Function to broadcast player positions to all 2D platformer clients
void broadcastPositions(zmq::socket_t& pubSocket) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::lock_guard<std::mutex> lock(playersMutex);

        if (!players.empty()) {
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerPosition)));
            char* buffer = static_cast<char*>(update.data());

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

// Function to handle events (e.g., respawn for 2D platformer)
void handleEvents(zmq::socket_t& eventRepSocket) {
    while (true) {
        zmq::message_t request;

        try {
            zmq::recv_result_t received = eventRepSocket.recv(request, zmq::recv_flags::none);
            if (received) {
                int clientId;
                SpawnEventData spawnData;
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&spawnData, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(spawnData));

                std::lock_guard<std::mutex> lock(playersMutex);

                if (players.find(clientId) != players.end() && players[clientId].gameType == PLATFORMER) {
                    // Adjust spawn point if it's blocked
                    for (const auto& player : players) {
                        if (std::abs(player.second.pos.x - spawnData.spawnX) <= 25 &&
                            std::abs(player.second.pos.y - spawnData.spawnY) <= 25) {
                            spawnData.spawnX += 60;  // Shift spawn point
                        }
                    }

                    zmq::message_t reply(sizeof(spawnData));
                    memcpy(reply.data(), &spawnData, sizeof(spawnData));
                    eventRepSocket.send(reply, zmq::send_flags::none);
                }
            }
        }
        catch (const zmq::error_t& e) {
            std::cerr << "Error handling events: " << e.what() << std::endl;
        }
    }
}

// Function to check for disconnected clients
void checkForTimeouts() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(playersMutex);

        for (auto it = lastHeartbeat.begin(); it != lastHeartbeat.end();) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count() > HEARTBEAT_INTERVAL_MS) {
                int clientId = it->first;
                std::cout << "Client " << clientId << " disconnected." << std::endl;

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

int main() {
    zmq::context_t context(2);
    zmq::socket_t repSocket(context, zmq::socket_type::rep);
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);
    zmq::socket_t eventRepSocket(context, zmq::socket_type::rep);

    repSocket.bind("tcp://*:5555");
    pubSocket.bind("tcp://*:5556");
    eventRepSocket.bind("tcp://*:5557");

    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));
    std::thread timeoutThread(checkForTimeouts);
    std::thread eventThread(handleEvents, std::ref(eventRepSocket));

    requestThread.join();
    broadcastThread.join();
    timeoutThread.join();
    eventThread.join();

    return 0;
}
