#pragma once
#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "Event.h"

enum InputAction {
    MOVE_LEFT,
    MOVE_RIGHT,
	MOVE_UP,
	MOVE_DOWN,
    JUMP,
    STOP
};

/**
 * @brief Represents an input event triggered by player actions.
 */
class InputEvent : public Event {
public:
    InputEvent(int objectID, const InputAction& inputAction, Timeline* timeline);

    int getObjectID() const;
    InputAction getInputAction() const;

private:
    int objectID;      // ID of the object receiving input
    InputAction inputAction; // Type of input action (e.g., "MoveLeft", "Jump")
};

#endif // INPUT_EVENT_H
