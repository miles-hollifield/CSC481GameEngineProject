#include "InputEvent.h"

InputEvent::InputEvent(int objectID, const std::string& inputType)
    : Event("Input", 4), objectID(objectID), inputType(inputType) {}

int InputEvent::getObjectID() const {
    return objectID;
}

std::string InputEvent::getInputType() const {
    return inputType;
}
