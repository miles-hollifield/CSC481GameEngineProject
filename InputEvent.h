#pragma once
#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "Event.h"

/**
 * @brief Represents an input event triggered by player actions.
 */
class InputEvent : public Event {
public:
    InputEvent(int objectID, const EventType& inputType);

    int getObjectID() const;
    EventType getInputType() const;
	Event getEvent() const;

private:
    int objectID;      // ID of the object receiving input
    EventType inputType; // Type of input (e.g., "MoveLeft", "Jump")
};

#endif // INPUT_EVENT_H
