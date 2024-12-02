#pragma once
#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "Event.h"

/**
 * @brief Enumeration of possible input actions in the game.
 *
 * Defines the various actions a player can take, such as moving left or right,
 * jumping, or stopping movement. These actions are used to trigger input events.
 */
enum InputAction {
    MOVE_LEFT,  // Represents the player moving to the left.
    MOVE_RIGHT, // Represents the player moving to the right.
	MOVE_UP,    // Represents the player moving up.
	MOVE_DOWN,  // Represents the player moving down.
    JUMP,       // Represents the player jumping.
    STOP        // Represents the player stopping movement.
};

/**
 * @brief Represents an input event triggered by player actions.
 *
 * This event is raised when a player initiates an action (e.g., moving, jumping),
 * and it holds the information necessary to process the action in the game.
 */
class InputEvent : public Event {
public:
    /**
     * @brief Constructs an InputEvent with the specified parameters.
     *
     * @param objectID The ID of the object receiving the input action.
     * @param inputAction The specific action performed by the player.
     * @param timeline Pointer to the timeline used for timestamping the event.
     */
    InputEvent(int objectID, const InputAction& inputAction, Timeline* timeline);

    /**
     * @brief Retrieves the ID of the object associated with this input event.
     *
     * @return The object ID related to this input event.
     */
    int getObjectID() const;

    /**
     * @brief Retrieves the input action associated with this event.
     *
     * @return The InputAction representing the player's action.
     */
    InputAction getInputAction() const;

private:
    int objectID;            // ID of the object receiving the input.
    InputAction inputAction; // Type of input action (e.g., MOVE_LEFT, JUMP).
};

#endif // INPUT_EVENT_H
