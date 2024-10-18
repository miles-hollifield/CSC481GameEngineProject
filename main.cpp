#include "main.h"
#include "game.h"  // Include Game2 header for the property-based model
#include <iostream>
#include <zmq.hpp>
#include <thread>
#include "defs.h"

// Store global player ID (unique for each client)
int playerId = -1;

// Function to receive updates from the server
void receiveUpdates(Game& game, zmq::socket_t& subSocket) {
    while (true) {
        zmq::message_t update;
        subSocket.recv(update, zmq::recv_flags::none);

        const char* buffer = static_cast<const char*>(update.data());
        size_t numPlayers = (update.size() - 2 * sizeof(PlayerPosition)) / sizeof(PlayerPosition);

        for (size_t i = 0; i < numPlayers; ++i) {
            PlayerPosition pos;
            memcpy(&pos, buffer, sizeof(PlayerPosition));
            game.updatePlayerPosition(i, pos);  // Update player position in the game
            buffer += sizeof(PlayerPosition);
        }

        // Deserialize platform positions from the server
        PlayerPosition platformPosition, verticalPlatformPosition;
        memcpy(&platformPosition, buffer, sizeof(platformPosition));
        memcpy(&verticalPlatformPosition, buffer + sizeof(platformPosition), sizeof(verticalPlatformPosition));

        game.updatePlatformPosition(platformPosition);
        game.updateVerticalPlatformPosition(verticalPlatformPosition);
    }
}

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

    // ZeroMQ context and sockets setup for networking
    zmq::context_t context(1);
    zmq::socket_t reqSocket(context, zmq::socket_type::req);  // Request socket for player data
    zmq::socket_t subSocket(context, zmq::socket_type::sub);  // Subscription socket for updates

    reqSocket.connect("tcp://localhost:5555");
    subSocket.connect("tcp://localhost:5556");
    subSocket.set(zmq::sockopt::subscribe, "");

    // Create an instance of the Game class, passing the SDL renderer
    Game game(renderer);

    // Thread for receiving updates from the server
    std::thread updateThread(receiveUpdates, std::ref(game), std::ref(subSocket));

    // Start the game loop
    game.run(reqSocket);

    // Clean up SDL resources after the game loop ends
    close(window, renderer);

    // Wait for the update thread to finish
    updateThread.join();

    return 0;
}
