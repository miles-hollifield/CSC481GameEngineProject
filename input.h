#ifndef INPUT_H
#define INPUT_H

#include "SDL2/SDL.h"
#include "PhysicsEntity.h"
#include "defs.h"
#include "Timeline.h"

/**
 * InputHandler is responsible for handling keyboard inputs and updating the
 * positions of controllable game entities. It also manages game pause, unpause,
 * and time scaling through the timeline.
 */
class InputHandler {
public:
    /**
     * Handle input for a controllable entity. Adjusts the position of the entity
     * based on the keyboard input while respecting the timeline's state.
     *
     * Movement is adjusted based on the provided frame delta and entity speed.
     *
     * @param entity The game entity to control, typically the player character.
     * @param moveSpeed The base movement speed of the entity, in pixels per second.
     * @param frameDelta The time elapsed since the last frame, in seconds.
     * @param timeline The timeline controlling the flow of time, to ensure input is
     *                 ignored if the game is paused.
     */
    void handleInput(PhysicsEntity& entity, int moveSpeed, float frameDelta, const Timeline& timeline);

    /**
     * Handle SDL events related to pausing, unpausing, and time scaling (changing game speed).
     * This function responds to specific keypresses that modify the state of the timeline.
     *
     * Keys and their effects:
     * - 'P': Pause the timeline.
     * - 'U': Unpause the timeline.
     * - '1': Set normal speed (1.0x).
     * - '2': Set half speed (0.5x).
     * - '3': Set double speed (2.0x).
     *
     * @param e The SDL_Event containing keyboard input events.
     * @param timeline The timeline object that controls the game's time flow,
     *                 which will be adjusted based on the events.
     */
    void handleEvent(const SDL_Event& e, Timeline& timeline);
};

#endif // INPUT_H
