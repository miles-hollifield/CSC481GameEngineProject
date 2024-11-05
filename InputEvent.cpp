#include "InputEvent.h"

InputEvent::InputEvent(int objectID, const std::string& inputAction, Timeline* timeline)
    : Event(INPUT, 4, timeline), objectID(objectID), inputAction(inputAction) {}

int InputEvent::getObjectID() const {
    return objectID;
}

std::string InputEvent::getInputAction() const {
    return inputAction;
}