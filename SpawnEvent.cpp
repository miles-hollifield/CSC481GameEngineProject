#include "SpawnEvent.h"

SpawnEvent::SpawnEvent(int objectID)
    : Event("Spawn", 3), objectID(objectID) {}

int SpawnEvent::getObjectID() const {
    return objectID;
}
