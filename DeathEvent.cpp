#include "DeathEvent.h"

DeathEvent::DeathEvent(int objectID)
    : Event("Death", 2), objectID(objectID) {}

int DeathEvent::getObjectID() const {
    return objectID;
}
