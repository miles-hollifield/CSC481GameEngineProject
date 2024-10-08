#include "PhysicsEntity.h"

// Constructor: Initialize position, size, and gravity
PhysicsEntity::PhysicsEntity(int x, int y, int w, int h, float gravityValue)
    : velocityY(0.0f), gravity(gravityValue), isOnGround(false) {
    rect = { x, y, w, h };
}

// Apply gravity to the entity
void PhysicsEntity::applyGravity(int screenHeight, float elapsedTime) {
    isOnGround = false;
    // Update velocity (pixels per second)
    velocityY += gravity;  // Increase vertical velocity by gravity * elapsedTime

    // Cap the downward velocity to prevent excessive speed
    if (velocityY > 2500.f) {
        velocityY = 2500.f;
    }

    // Update position based on velocity (pixels)
    rect.y += static_cast<int>(velocityY * elapsedTime);

    // Prevent the entity from falling through the bottom of the screen
    if (rect.y + rect.h > screenHeight) {
        rect.y = screenHeight - rect.h;  // Position the entity at the bottom edge
        velocityY = 0.0f;  // Stop downward velocity
        isOnGround = true;
    }
}

// Handle collision detection with a platform
void PhysicsEntity::handleCollision(const SDL_Rect& platformRect) {
    if (SDL_HasIntersection(&rect, &platformRect)) {
        if (rect.y + rect.h / 2 < platformRect.y) { // checks above
            velocityY = 0.0f; // Stops downward velocity
            rect.y = platformRect.y - rect.h; // Positions entity on top of platform
            isOnGround = true;
        }
        if (rect.y + rect.h / 2 > platformRect.y + platformRect.h) { // check below
            velocityY = gravity; // Sets downward velocity to positive (jumping is just set velocityY to negative)
            rect.y = platformRect.y + platformRect.h; // Positions entity below platform
        }
        if (rect.x + rect.w / 2 < platformRect.x) { // Checks left
            rect.x = platformRect.x - rect.w; // Positions entity to left of platform
        }
        if (rect.x + rect.w / 2 > platformRect.x + platformRect.w) { // Checks right
            rect.x = platformRect.x + platformRect.w; // Positions entity to right of platform
        }
    }
}
