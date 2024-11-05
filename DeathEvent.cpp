#include "DeathEvent.h"

DeathEvent::DeathEvent(int objectID)
    : Event(DEATH, 2), objectID(objectID) {}

int DeathEvent::getObjectID() const {
    return objectID;
}
