#include "InputEvent.h"

InputEvent::InputEvent(int objectID, const InputAction& inputAction, Timeline* timeline)
    : Event(INPUT, 4, timeline), objectID(objectID), inputAction(inputAction) {}

int InputEvent::getObjectID() const {
    return objectID;
}

InputAction InputEvent::getInputAction() const {
    return inputAction;
}