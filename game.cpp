#include "game.h"
#include "ThreadManager.h"
#include <iostream>
#include <cstring>
#include "input.h"

// Constructor for the Game class
Game::Game(SDL_Renderer* renderer, zmq::socket_t& reqSocket, zmq::socket_t& subSocket)
    : renderer(renderer), reqSocket(reqSocket), subSocket(subSocket), quit(false), playerId(-1),
    platformRect{ 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 3, 50 },  // Static platform
    movingPlatformRect{ SCREEN_WIDTH / 3, SCREEN_HEIGHT - 300, SCREEN_WIDTH / 4, 50 },  // Moving platform
    controllableRect(500, 400, 50, 50, 10.0f),  // Controllable player entity with gravity
    myPlayer{ 500, 400 },  // Initial player position
    movingPlatformVelocity(100.0f),  // Velocity of moving platform
    gameTimeline(nullptr, 1.0f),  // Initialize timeline with 1.0 speed (real-time)
    lastTime(std::chrono::steady_clock::now())  // Initialize last time for frame delta calculation
{
}

// Destructor for the Game class
Game::~Game() {
    // Any clean-up logic can go here
}

// Main game loop
void Game::run() {
    ThreadManager threadManager;

    // Start a separate thread to handle platform updates
    threadManager.createThread([this]() {
        auto lastTime = std::chrono::steady_clock::now();

        while (!quit) {
            // Calculate time elapsed (frame delta)
            auto currentTime = std::chrono::steady_clock::now();
            std::chrono::duration<float> deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            float frameDelta = deltaTime.count();

            // Only update platform if the game is not paused
            if (!gameTimeline.isPaused()) {
                updatePlatform(frameDelta);
            }

            // Sleep to limit CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        });

    // Main thread handles event polling, input, and rendering
    auto lastTime = std::chrono::steady_clock::now();

    while (!quit) {
        // Calculate time elapsed (frame delta)
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        float frameDelta = deltaTime.count();

        // Handle input and events
        handleEvents(frameDelta);

        // Receive player positions from the server
        receivePlayerPositions();

        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Update game state if not paused
        if (!gameTimeline.isPaused()) {
            update(frameDelta);
        }

        // Render the game
        render(renderer, allPlayers);

        // Present the updated screen to the user
        SDL_RenderPresent(renderer);

        // Cap frame rate to around 60 FPS
        SDL_Delay(16);
    }

    // Join all threads before exiting
    threadManager.joinAll();
}

// Handle events, including input and time-related events
void Game::handleEvents(float frameDelta) {
    InputHandler inputHandler;

    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        // Always handle pause/unpause and time scaling events
        inputHandler.handleEvent(e, gameTimeline);

        // Only handle player movement if the game is not paused
        if (!gameTimeline.isPaused()) {
            // Handle input for controllable player entity
            inputHandler.handleInput(controllableRect, 300, frameDelta, gameTimeline);
        }

        // Send movement updates to the server (if not paused)
        sendMovementUpdate();
    }
}

// Send the current player's position to the server
void Game::sendMovementUpdate() {
    if (!gameTimeline.isPaused()) {
        zmq::message_t request(sizeof(playerId) + sizeof(myPlayer));

        // Package the player ID and position
        memcpy(request.data(), &playerId, sizeof(playerId));
        memcpy(static_cast<char*>(request.data()) + sizeof(playerId), &myPlayer, sizeof(myPlayer));

        // Send the data to the server
        reqSocket.send(request, zmq::send_flags::none);

        // Receive acknowledgment from the server
        zmq::message_t reply;
        reqSocket.recv(reply);

        // If the player ID is not assigned, get the assigned ID from the server
        if (playerId == -1) {
            memcpy(&playerId, reply.data(), sizeof(playerId));
            std::cout << "Received assigned playerId: " << playerId << std::endl;
        }
    }
}

// Receive player positions from the server
void Game::receivePlayerPositions() {
    zmq::message_t update;
    zmq::recv_result_t recvResult = subSocket.recv(update, zmq::recv_flags::dontwait);

    if (recvResult) {
        // Only update player positions if the game is not paused
        if (!gameTimeline.isPaused()) {
            allPlayers.clear();
            char* buffer = static_cast<char*>(update.data());

            // Deserialize player positions
            while (buffer < static_cast<char*>(update.data()) + update.size() - sizeof(PlayerPosition)) {
                int id;
                PlayerPosition pos;

                memcpy(&id, buffer, sizeof(id));
                buffer += sizeof(id);
                memcpy(&pos, buffer, sizeof(pos));
                buffer += sizeof(pos);

                allPlayers[id] = pos;
            }

            // Update platform position
            memcpy(&movingPlatformRect, buffer, sizeof(PlayerPosition));
        }
    }
}

// Update the platform's movement in a separate thread
void Game::updatePlatform(float frameDelta) {
    if (gameTimeline.isPaused()) {
        return;  // Skip update if paused
    }

    std::lock_guard<std::mutex> lock(platformMutex);  // Ensure thread safety

    // Move the platform based on velocity and frame delta
    movingPlatformRect.x += static_cast<int>(movingPlatformVelocity * frameDelta);

    // Reverse direction if the platform hits the screen's edge
    if (movingPlatformRect.x <= 0 || movingPlatformRect.x >= SCREEN_WIDTH - movingPlatformRect.w) {
        movingPlatformVelocity = -movingPlatformVelocity;
    }
}

// Update the game state, including player movement and collisions
void Game::update(float frameDelta) {
    controllableRect.applyGravity(SCREEN_HEIGHT, frameDelta);

    // Handle collisions with static and moving platforms
    controllableRect.handleCollision(platformRect);
    controllableRect.handleCollision(movingPlatformRect);

    // Update player's position
    myPlayer.x = controllableRect.rect.x;
    myPlayer.y = controllableRect.rect.y;
}

// Render the game objects on the screen
void Game::render(SDL_Renderer* renderer, const std::unordered_map<int, PlayerPosition>& playerPositions) {
    // Clear screen with a blue background
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_RenderClear(renderer);

    // Render static platform (red)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &platformRect);

    // Render moving platform (cyan)
    std::lock_guard<std::mutex> lock(platformMutex);  // Lock the mutex for thread safety
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_RenderFillRect(renderer, &movingPlatformRect);

    // Render controllable player entity (yellow)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderFillRect(renderer, &controllableRect.rect);

    // Render other players (green)
    for (const auto& player : playerPositions) {
        int id = player.first;

        // Avoid rendering the local player twice
        if (id != playerId) {
            PlayerPosition pos = player.second;

            // Create or update player rectangles
            if (allRects.find(id) == allRects.end()) {
                allRects[id] = { pos.x, pos.y, 50, 50 };
            }
            else if (!gameTimeline.isPaused()) {
                allRects[id].x = pos.x;
                allRects[id].y = pos.y;
            }

            // Render other players
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Green for other players
            SDL_RenderFillRect(renderer, &allRects[id]);
        }
    }
}
