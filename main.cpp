#include "main.h"
#include "game.h"  // Include Game2 header for the property-based model
#include <iostream>

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

    // Create an instance of the Game2 class, passing the SDL renderer
    Game game(renderer);  // No need to pass additional arguments (only renderer is passed)

    // Start the game loop
    game.run();

    // Clean up SDL resources after the game loop ends
    close(window, renderer);  // Clean up SDL resources by destroying the window and renderer

    return 0;  // Exit the program successfully
}