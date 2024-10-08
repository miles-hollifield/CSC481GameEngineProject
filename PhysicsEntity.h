#ifndef PHYSICSENTITY_H
#define PHYSICSENTITY_H

#include "SDL2/SDL.h"

/**
 * PhysicsEntity class is responsible for handling physics-related properties
 * and behaviors for game entities, including gravity and collision detection.
 */
class PhysicsEntity {
public:
    SDL_Rect rect; // The rectangle representing the entity's position and size
    float velocityY; // Vertical velocity, affected by gravity
    float gravity; // Gravity value, applied to the entity
    bool isOnGround; // Indicates if the entity is on the ground or a platform

    /**
     * Constructor for PhysicsEntity.
     * Initializes the entity with a position, size, and gravity value.
     * @param x The x-coordinate of the entity.
     * @param y The y-coordinate of the entity.
     * @param w The width of the entity.
     * @param h The height of the entity.
     * @param gravityValue The gravity value to be applied to the entity.
     */
    PhysicsEntity(int x, int y, int w, int h, float gravityValue);

    /**
     * Apply gravity to the entity.
     * Increases the vertical velocity over time and updates the entity's position.
     * @param screenHeight The height of the screen, used to ensure the entity does not fall off-screen.
     * @param elapsedTime The elapsed time since the last frame.
     */
    void applyGravity(int screenHeight, float elapsedTime);

    /**
     * Handle collision detection with a platform.
     * If a collision is detected, the entity is positioned to the appropriate side of the platform, if it is on above or below, its vertical velocity is reset.
     * @param platformRect The SDL_Rect representing the platform to check for collisions.
     */
    void handleCollision(const SDL_Rect& platformRect);
};

#endif
