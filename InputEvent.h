#pragma once
#ifndef INPUT_EVENT_H
#define INPUT_EVENT_H

#include "Event.h"

/**
 * @brief Represents an input event triggered by player actions.
 */
class InputEvent : public Event {
public:
    InputEvent(int objectID, const std::string& inputAction, Timeline* timeline);

    int getObjectID() const;
    std::string getInputAction() const;

private:
    int objectID;      // ID of the object receiving input
    std::string inputAction; // Type of input action (e.g., "MoveLeft", "Jump")
};

#endif // INPUT_EVENT_H
