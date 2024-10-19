#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>
#include "../Timeline.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Define player position structure
struct PlayerPosition {
    int x, y;
};

// Global variables for players, platform position, mutex, and client tracking
std::unordered_map<int, PlayerPosition> players;  // Map of players and their positions
std::mutex playersMutex;  // Mutex to ensure thread-safe access to player data
int nextClientId = 0;  // Unique ID for each new player

// Moving platform variables
PlayerPosition platformPosition = { 150, 900 };  // Initial position of the horizontal moving platform
PlayerPosition verticalPlatformPosition = { 300, 100 };  // Initial position of the vertical moving platform
float platformVelocityX = 2.0f;  // Horizontal velocity
float platformVelocityY = 2.0f;  // Vertical velocity


// Server's own timeline for the platform
Timeline platformTimeline(nullptr, 1.0f);  // Server-side time scaling for shared entities (e.g., moving platform)

// Function to handle incoming requests from clients
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        // Check for new client connections
        zmq::message_t request;
        zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::dontwait);
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
                std::cout << "New player connected: " << clientId << std::endl;

                // Send the new clientId back to the client
                zmq::message_t reply(sizeof(clientId));
                memcpy(reply.data(), &clientId, sizeof(clientId));
                repSocket.send(reply, zmq::send_flags::none);
            }
            else {
                // Update the existing player's position
                players[clientId] = pos;

                // Send acknowledgment
                zmq::message_t reply("OK", 2);
                repSocket.send(reply, zmq::send_flags::none);
            }
        }
        // Sleep for a short period to simulate update intervals
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

//platformPosition.x += static_cast<int>(platformVelocity * deltaTime / platformTimeline.getTic());
// Function to update platform positions based on velocity
void updatePlatformPositions(float deltaTime) {
    // Update horizontal platform position
    platformPosition.x += static_cast<int>(platformVelocityX * deltaTime / platformTimeline.getTic());

    // Reverse direction if the horizontal platform reaches screen edges
    if (platformPosition.x <= 0 || platformPosition.x >= SCREEN_WIDTH - 200) {  // Assume platform width is 200
        platformVelocityX = -platformVelocityX;
    }

    // Update vertical platform position
    verticalPlatformPosition.y += static_cast<int>(platformVelocityY * deltaTime / platformTimeline.getTic());

    // Reverse direction if the vertical platform reaches screen edges
    if (verticalPlatformPosition.y <= 0 || verticalPlatformPosition.y >= SCREEN_HEIGHT - 50) {  // Assume platform height is 50
        platformVelocityY = -platformVelocityY;
    }
}


// Broadcast platform and player positions, include velocities
void broadcastPositions(zmq::socket_t& pubSocket) {
    auto lastTime = std::chrono::steady_clock::now();  // Track time for delta calculation

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Adjust as needed

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Update platform position based on server's deltaTime
        updatePlatformPositions(deltaTime.count());

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex to ensure thread safety
        if (!players.empty()) {
            // Prepare a buffer for broadcasting player positions + platform position + velocity
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerPosition)) + sizeof(PlayerPosition) + 2 * sizeof(float));  // Add velocity size
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

            // Now, serialize platform position and velocity at the end of the message
            std::memcpy(buffer, &platformPosition, sizeof(platformPosition));
            buffer += sizeof(platformPosition);
            std::memcpy(buffer, &platformVelocityX, sizeof(platformVelocityX));  // Add velocity to the message
            std::memcpy(buffer, &platformVelocityY, sizeof(platformVelocityY));

            // Send the update to all clients
            pubSocket.send(update, zmq::send_flags::none);
        }
    }
}

int main() {
    zmq::context_t context(2);  // Initialize ZeroMQ context
    zmq::socket_t repSocket(context, zmq::socket_type::rep);  // For client connection
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);  // For broadcasting player positions

    repSocket.bind("tcp://*:5555");  // Bind to port 5555 for client requests
    pubSocket.bind("tcp://*:5556");  // Bind to port 5556 for broadcasting positions

    // Create threads for handling requests and broadcasting positions
    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));

    // Wait for threads to finish (in practice, they won't terminate)
    requestThread.join();
    broadcastThread.join();

    return 0;
}