#include "SpawnEvent.h"

SpawnEvent::SpawnEvent(int objectID)
    : Event(SPAWN, 3), objectID(objectID) {}

int SpawnEvent::getObjectID() const {
    return objectID;
}
