#include "main.h"
#include <zmq.hpp>
#include <iostream>
#include <unordered_map>
#include <cstring>

// Store global player ID (unique for each client)
int playerId = -1;

int main(int argc, char* args[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Initialize SDL and create window and renderer
    init(window, renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    // Check if initialization of SDL, window, or renderer failed
    if (window == nullptr || renderer == nullptr) {
        std::cerr << "Initialization failed!" << std::endl;
        return -1;
    }

    // Initialize ZeroMQ for client communication
    // Source: https://zguide.zeromq.org/docs/
    zmq::context_t context(1);  // ZeroMQ context with a single IO thread
    zmq::socket_t reqSocket(context, zmq::socket_type::req);  // For sending movement updates to the server
    zmq::socket_t subSocket(context, zmq::socket_type::sub);  // For receiving player positions from the server

    // Connect to the server using ZeroMQ
    reqSocket.connect("tcp://localhost:5555");  // Connect request socket to server
    subSocket.connect("tcp://localhost:5556");  // Connect subscription socket to server
    subSocket.set(zmq::sockopt::subscribe, "");  // Subscribe to all messages on the subscription socket

    // Create an instance of the Game class, passing the SDL renderer and ZeroMQ sockets
    Game game(renderer, reqSocket, subSocket);

    // Start the game loop
    game.run();

    // Clean up ZeroMQ and SDL resources after the game loop ends
    reqSocket.close();  // Close the request socket
    subSocket.close();  // Close the subscription socket
    context.shutdown();  // Shutdown the ZeroMQ context

    close(window, renderer);  // Clean up SDL resources by destroying the window and renderer

    return 0;  // Exit the program successfully
}