#include "input.h"
#include <iostream>

// Handle input for a controllable entity with time scaling
void InputHandler::handleInput(PhysicsEntity& entity, int moveSpeed, float frameDelta, const Timeline& timeline) {
    // Ensure no input is processed when the game is paused
    if (timeline.isPaused()) {
        return;  // Skip input handling if the game is paused
    }

    // Get the current state of the keyboard
    const Uint8* state = SDL_GetKeyboardState(NULL);

    // Adjust the movement speed based on the frame delta time
    float scaledMoveSpeed = moveSpeed * frameDelta;

    // Move the entity left if the left arrow key is pressed and the entity is within bounds
    if (state[SDL_SCANCODE_LEFT] && entity.rect.x > 0) {
        entity.rect.x -= static_cast<int>(scaledMoveSpeed);  // Move left
    }

    // Move the entity right if the right arrow key is pressed and the entity is within bounds
    if (state[SDL_SCANCODE_RIGHT] && entity.rect.x < SCREEN_WIDTH - entity.rect.w) {
        entity.rect.x += static_cast<int>(scaledMoveSpeed);  // Move right
    }

    // Handle jumping if the up arrow key is pressed
    if (state[SDL_SCANCODE_UP]) {
        // If the entity is on the ground, allow it to jump
        if (entity.isOnGround) {
            entity.velocityY = -35.0f * entity.gravity;  // Set jump velocity (upward)
            entity.isOnGround = false;  // Entity is no longer on the ground
        }
    }
}

// Handle SDL events for pausing, unpausing, and time scaling
void InputHandler::handleEvent(const SDL_Event& e, Timeline& timeline) {
    // Only process keydown events
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_p:  // Press 'P' to pause
            timeline.pause();
            std::cout << "Paused timeline" << std::endl;
            break;
        case SDLK_u:  // Press 'U' to unpause
            timeline.unpause();
            std::cout << "Unpaused timeline" << std::endl;
            break;
        case SDLK_1:  // Press '1' for normal speed (1.0x)
            timeline.changeTic(1.0f);
            std::cout << "Changed tic to normal speed (1.0x)" << std::endl;
            break;
        case SDLK_2:  // Press '2' for half speed (0.5x)
            timeline.changeTic(2.0f);
            std::cout << "Changed tic to half speed (0.5x)" << std::endl;
            break;
        case SDLK_3:  // Press '3' for double speed (2.0x)
            timeline.changeTic(0.5f);
            std::cout << "Changed tic to double speed (2.0x)" << std::endl;
            break;
        default:
            break;
        }
    }
}
