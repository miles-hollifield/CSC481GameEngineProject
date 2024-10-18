#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

// Define player position structure
struct PlayerPosition {
    int x, y;
};

// Global variables for players, platform position, mutex, and client tracking
std::unordered_map<std::string, PlayerPosition> players;  // Map of players and their positions, identified by a unique client address
std::mutex playersMutex;  // Mutex to ensure thread-safe access to player data
int nextClientId = 0;  // Unique ID for each new player

// Moving platform variables
PlayerPosition platformPosition = { 150, 900 };  // Initial position of the horizontal moving platform
PlayerPosition verticalPlatformPosition = { 300, 100 };  // Initial position of the vertical moving platform
float platformVelocityX = 2.0f;  // Horizontal velocity
float platformVelocityY = 2.0f;  // Vertical velocity

// Function to handle incoming requests from clients
void handleRequests(zmq::socket_t& routerSocket) {
    while (true) {
        zmq::message_t clientAddr;
        zmq::message_t request;

        // Receive client address and request in multipart message
        if (routerSocket.recv(clientAddr, zmq::recv_flags::none) &&
            routerSocket.recv(request, zmq::recv_flags::none)) {

            if (request.size() != sizeof(PlayerPosition)) {
                std::cerr << "Received invalid request size from client!" << std::endl;
                continue;
            }

            // Extract player position from the request
            PlayerPosition pos;
            memcpy(&pos, request.data(), sizeof(pos));

            // Convert the client address to a string
            std::string clientId(static_cast<char*>(clientAddr.data()), clientAddr.size());

            // Lock the mutex for thread safety
            {
                std::lock_guard<std::mutex> lock(playersMutex);
                players[clientId] = pos;  // Update the player's position
            }

            // Send acknowledgment to the client
            zmq::message_t replyAddr(clientAddr.data(), clientAddr.size());
            zmq::message_t reply("OK", 2);  // Send OK as a response to the client

            try {
                routerSocket.send(replyAddr, zmq::send_flags::sndmore);
                routerSocket.send(reply, zmq::send_flags::none);
            }
            catch (const zmq::error_t& ex) {
                std::cerr << "Failed to send acknowledgment to client: " << ex.what() << std::endl;
            }
        }
        else {
            std::cerr << "Failed to receive data from client!" << std::endl;
        }
    }
}


// Function to update platform positions based on velocity
void updatePlatformPositions() {
    // Update horizontal platform position
    platformPosition.x += static_cast<int>(platformVelocityX);

    // Reverse direction if the horizontal platform reaches screen edges
    if (platformPosition.x <= 0 || platformPosition.x >= SCREEN_WIDTH - 200) {  // Assume platform width is 200
        platformVelocityX = -platformVelocityX;
    }

    // Update vertical platform position
    verticalPlatformPosition.y += static_cast<int>(platformVelocityY);

    // Reverse direction if the vertical platform reaches screen edges
    if (verticalPlatformPosition.y <= 0 || verticalPlatformPosition.y >= SCREEN_HEIGHT - 50) {  // Assume platform height is 50
        platformVelocityY = -platformVelocityY;
    }
}

// Function to broadcast player positions and platform positions to all clients
void broadcastPositions(zmq::socket_t& pubSocket) {
    auto lastTime = std::chrono::steady_clock::now();  // Track time for delta calculation

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Reduced update interval to 50ms

        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Update platform positions
        updatePlatformPositions();

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex to ensure thread safety

        if (!players.empty()) {
            // Prepare a buffer for broadcasting player positions + platform positions
            zmq::message_t update(players.size() * (sizeof(PlayerPosition)) + 2 * sizeof(PlayerPosition));
            char* buffer = static_cast<char*>(update.data());

            // Serialize all players' data (PlayerPosition)
            for (const auto& player : players) {
                PlayerPosition pos = player.second;

                // Copy PlayerPosition to the buffer
                std::memcpy(buffer, &pos, sizeof(pos));
                buffer += sizeof(pos);
            }

            // Now, serialize both platform positions at the end of the message
            std::memcpy(buffer, &platformPosition, sizeof(platformPosition));
            buffer += sizeof(platformPosition);
            std::memcpy(buffer, &verticalPlatformPosition, sizeof(verticalPlatformPosition));

            // Send the update to all clients
            try {
                pubSocket.send(update, zmq::send_flags::none);
            }
            catch (const zmq::error_t& ex) {
                std::cerr << "Failed to send update to clients: " << ex.what() << std::endl;
            }
        }
    }
}


int main() {
    zmq::context_t context(2);  // Initialize ZeroMQ context
    zmq::socket_t routerSocket(context, zmq::socket_type::router);  // For client connection
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);  // For broadcasting player positions

    routerSocket.bind("tcp://*:5555");  // Bind to port 5555 for client requests
    pubSocket.bind("tcp://*:5556");  // Bind to port 5556 for broadcasting positions

    // Create threads for handling requests and broadcasting positions
    std::thread requestThread(handleRequests, std::ref(routerSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));

    // Wait for threads to finish (in practice, they won't terminate)
    requestThread.join();
    broadcastThread.join();

    return 0;
}
