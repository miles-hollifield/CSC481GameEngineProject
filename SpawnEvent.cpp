#include "SpawnEvent.h"

SpawnEvent::SpawnEvent(int objectID, Timeline* timeline)
    : Event(SPAWN, 3, timeline), objectID(objectID) {}

int SpawnEvent::getObjectID() const {
    return objectID;
}
