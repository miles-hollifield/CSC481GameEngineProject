#pragma once
#include "defs.h"
#include "SDL2/SDL.h"
#include <iostream>

/**
 * Initializes SDL, creates a window, and a renderer.
 *
 * @param window Reference to the SDL_Window pointer that will be created.
 * @param renderer Reference to the SDL_Renderer pointer that will be created.
 * @param width The width of the window in pixels.
 * @param height The height of the window in pixels.
 *
 * @note If SDL fails to initialize or the window/renderer fails to be created,
 *       this function will print an error message.
 */
void init(SDL_Window*& window, SDL_Renderer*& renderer, int width, int height);

/**
 * Cleans up SDL resources by destroying the renderer and window.
 *
 * @param window Pointer to the SDL_Window that will be destroyed.
 * @param renderer Pointer to the SDL_Renderer that will be destroyed.
 *
 * @note After this function is called, SDL will be completely shutdown.
 */
void close(SDL_Window* window, SDL_Renderer* renderer);
