#include "init.h"

// Initialize SDL2 for creating a window and renderer
// Source: https://wiki.libsdl.org/SDL2/FrontPage
void init(SDL_Window*& window, SDL_Renderer*& renderer, int width, int height) {
    // Initialize SDL with video support
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // If initialization fails, print the SDL error
        std::cerr << "SDL2 could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Create the main window with the specified width and height, centered on the screen
    window = SDL_CreateWindow("CSC481 Team Engine",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN);

    // If window creation fails, print the SDL error
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    // Create the renderer associated with the window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // If renderer creation fails, print the SDL error
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // Set the draw color for clearing the screen (background color)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
}

// Clean up and close the SDL resources (window and renderer)
void close(SDL_Window* window, SDL_Renderer* renderer) {
    // Destroy the renderer
    SDL_DestroyRenderer(renderer);

    // Destroy the window
    SDL_DestroyWindow(window);

    // Quit SDL2 subsystems
    SDL_Quit();
}
