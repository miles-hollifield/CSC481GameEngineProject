#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define HEARTBEAT_INTERVAL_MS 2000  // Time after which the server assumes a client has disconnected

// Define player position structure
struct PlayerPosition {
    int x, y;
};

// Global variables for players, mutex, and client tracking
std::unordered_map<int, PlayerPosition> players;  // Map of players and their positions
std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeat; // Track last heartbeat time for each client
std::mutex playersMutex;  // Mutex to ensure thread-safe access to player data
int nextClientId = 0;  // Unique ID for each new player

// Function to handle incoming requests from clients
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        zmq::message_t request;

        try {
            // Check for new client connections or updates
            zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::none);

            if (received) {
                // Extract player position from the request
                int clientId;
                PlayerPosition pos;
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&pos, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(pos));

                std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex for thread safety
                if (clientId == -1) {
                    // Assign a new clientId for new connections
                    clientId = nextClientId++;
                    players[clientId] = pos;
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();  // Record the heartbeat time
                    std::cout << "New player connected: " << clientId << std::endl;

                    // Send the new clientId back to the client
                    zmq::message_t reply(sizeof(clientId));
                    memcpy(reply.data(), &clientId, sizeof(clientId));
                    repSocket.send(reply, zmq::send_flags::none);
                }
                else {
                    // Update the existing player's position and record heartbeat time
                    players[clientId] = pos;
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();  // Update heartbeat time

                    // Send acknowledgment
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

// Broadcast player positions to clients (no platforms involved)
void broadcastPositions(zmq::socket_t& pubSocket) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Adjust as needed

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex to ensure thread safety
        if (!players.empty()) {
            // Prepare a buffer for broadcasting player positions
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerPosition)));
            char* buffer = static_cast<char*>(update.data());

            // Serialize all players' data (clientId and PlayerPosition)
            for (const auto& player : players) {
                int id = player.first;
                PlayerPosition pos = player.second;

                // Copy clientId and PlayerPosition to the buffer
                std::memcpy(buffer, &id, sizeof(id));
                buffer += sizeof(id);
                std::memcpy(buffer, &pos, sizeof(pos));
                buffer += sizeof(pos);
            }

            // Send the update to all clients
            pubSocket.send(update, zmq::send_flags::none);
        }
    }
}

// Function to check for client disconnections based on heartbeat timeouts
void checkForTimeouts() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Check every second

        auto now = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex for thread safety
        for (auto it = lastHeartbeat.begin(); it != lastHeartbeat.end();) {
            int clientId = it->first;
            auto lastHeartbeatTime = it->second;

            // Check if the heartbeat has timed out
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeatTime).count() > HEARTBEAT_INTERVAL_MS) {
                std::cout << "Client " << clientId << " disconnected due to timeout." << std::endl;
                players.erase(clientId);  // Remove player from game world
                it = lastHeartbeat.erase(it);  // Remove client from heartbeat tracking
            }
            else {
                ++it;
            }
        }
    }
}

int main() {
    zmq::context_t context(2);  // Initialize ZeroMQ context
    zmq::socket_t repSocket(context, zmq::socket_type::rep);  // For client connection
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);  // For broadcasting player positions

    repSocket.bind("tcp://*:5555");  // Bind to port 5555 for client requests
    pubSocket.bind("tcp://*:5556");  // Bind to port 5556 for broadcasting positions

    // Create threads for handling requests, broadcasting positions, and checking for timeouts
    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));
    std::thread timeoutThread(checkForTimeouts);

    // Wait for threads to finish (in practice, they won't terminate)
    requestThread.join();
    broadcastThread.join();
    timeoutThread.join();

    return 0;
}
