#include "InputEvent.h"

InputEvent::InputEvent(int objectID, const EventType& inputType)
    : Event(INPUT, 4), objectID(objectID), inputType(inputType) {}

int InputEvent::getObjectID() const {
    return objectID;
}

EventType InputEvent::getInputType() const {
    return inputType;
}

Event InputEvent::getEvent() const {
	return Event(getType(), getPriority());
}
