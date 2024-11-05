#include "DeathEvent.h"

DeathEvent::DeathEvent(int objectID, Timeline* timeline)
    : Event(DEATH, 2, timeline), objectID(objectID) {}

int DeathEvent::getObjectID() const {
    return objectID;
}
