#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define HEARTBEAT_INTERVAL_MS 10000  // Time after which the server assumes a client has disconnected

// Define player position structure that holds x and y coordinates of the player
struct PlayerPosition {
    int x, y;
};

// Global variables for tracking players, heartbeat times, and a mutex for thread safety
std::unordered_map<int, PlayerPosition> players;  // Map of players and their positions
std::unordered_map<int, std::chrono::steady_clock::time_point> lastHeartbeat; // Map to track the last heartbeat time for each client
std::mutex playersMutex;  // Mutex to ensure thread-safe access to player data
int nextClientId = 0;  // Unique ID for each new player that connects

// Function to handle incoming requests from clients
void handleRequests(zmq::socket_t& repSocket) {
    while (true) {
        zmq::message_t request;

        try {
            // Receive messages from clients (blocking call)
            zmq::recv_result_t received = repSocket.recv(request, zmq::recv_flags::none);

            if (received) {
                // Extract client ID and player position from the request
                int clientId;
                PlayerPosition pos;
                memcpy(&clientId, request.data(), sizeof(clientId));
                memcpy(&pos, static_cast<char*>(request.data()) + sizeof(clientId), sizeof(pos));

                // Lock the mutex to safely update the shared player data
                std::lock_guard<std::mutex> lock(playersMutex);

                if (clientId == -1) {  // If this is a new client (ID -1)
                    // Assign a new unique clientId for the client
                    clientId = nextClientId++;
                    players[clientId] = pos;  // Store player's position
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();  // Record the client's heartbeat time
                    std::cout << "New player connected: " << clientId << std::endl;

                    // Send the new clientId back to the client
                    zmq::message_t reply(sizeof(clientId));
                    memcpy(reply.data(), &clientId, sizeof(clientId));
                    repSocket.send(reply, zmq::send_flags::none);
                }
                else {  // Existing client
                    // Update the player's position and reset their heartbeat time
                    players[clientId] = pos;
                    lastHeartbeat[clientId] = std::chrono::steady_clock::now();  // Update heartbeat time

                    // Send acknowledgment message to the client
                    zmq::message_t reply("OK", 2);
                    repSocket.send(reply, zmq::send_flags::none);
                }
            }
        }
        catch (const zmq::error_t& e) {
            // Handle any ZeroMQ errors that might occur during message reception
            std::cerr << "Error receiving message: " << e.what() << std::endl;
        }
    }
}

// Function to broadcast player positions to all clients
void broadcastPositions(zmq::socket_t& pubSocket) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Wait before each broadcast

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex to safely access player data
        if (!players.empty()) {
            // Prepare a buffer to hold the broadcast data (player IDs and their positions)
            zmq::message_t update(players.size() * (sizeof(int) + sizeof(PlayerPosition)));
            char* buffer = static_cast<char*>(update.data());

            // Serialize each player's client ID and position into the buffer
            for (const auto& player : players) {
                int id = player.first;
                PlayerPosition pos = player.second;

                std::memcpy(buffer, &id, sizeof(id));  // Copy clientId to buffer
                buffer += sizeof(id);
                std::memcpy(buffer, &pos, sizeof(pos));  // Copy PlayerPosition to buffer
                buffer += sizeof(pos);
            }

            // Broadcast the serialized data to all clients
            pubSocket.send(update, zmq::send_flags::none);
        }
    }
}

// Function to check if any clients have timed out due to missed heartbeats
void checkForTimeouts() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Check every second

        auto now = std::chrono::steady_clock::now();  // Get the current time

        std::lock_guard<std::mutex> lock(playersMutex);  // Lock the mutex to safely modify player data
        for (auto it = lastHeartbeat.begin(); it != lastHeartbeat.end();) {
            int clientId = it->first;
            auto lastHeartbeatTime = it->second;

            // Check if the client's heartbeat has expired (indicating a disconnect)
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeatTime).count() > HEARTBEAT_INTERVAL_MS) {
                std::cout << "Client " << clientId << " disconnected due to timeout." << std::endl;
                players.erase(clientId);  // Remove the player from the game world
                it = lastHeartbeat.erase(it);  // Remove the client from heartbeat tracking
            }
            else {
                ++it;  // Move to the next player
            }
        }
    }
}

int main() {
    zmq::context_t context(2);  // Initialize ZeroMQ context with two IO threads
    zmq::socket_t repSocket(context, zmq::socket_type::rep);  // REP socket for receiving client requests
    zmq::socket_t pubSocket(context, zmq::socket_type::pub);  // PUB socket for broadcasting player positions

    // Bind sockets to respective ports for client communication
    repSocket.bind("tcp://*:5555");  // Bind REP socket to port 5555 for receiving client requests
    pubSocket.bind("tcp://*:5556");  // Bind PUB socket to port 5556 for broadcasting player positions

    // Create and start threads to handle client requests, broadcast player positions, and check for timeouts
    std::thread requestThread(handleRequests, std::ref(repSocket));
    std::thread broadcastThread(broadcastPositions, std::ref(pubSocket));
    std::thread timeoutThread(checkForTimeouts);

    // Wait for the threads to finish (although in this case, the threads run indefinitely)
    requestThread.join();
    broadcastThread.join();
    timeoutThread.join();

    return 0;
}
