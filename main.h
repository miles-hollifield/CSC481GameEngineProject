#pragma once
#include "SDL2/SDL.h"
#include <zmq.hpp>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include "game3.h"
#include "init.h"
#include "defs.h"
#include "Timeline.h"
#include "ThreadManager.h"

/**
 * Main entry point for the application.
 *
 * @param argc The number of command-line arguments.
 * @param argv Array of command-line arguments.
 *
 * @return An integer value representing the exit status of the program.
 *         A return value of 0 indicates successful execution, while any other value indicates an error.
 *
 * @note This function initializes SDL, sets up the game loop, and cleans up resources after the game exits.
 */
int main(int argc, char* argv[]);