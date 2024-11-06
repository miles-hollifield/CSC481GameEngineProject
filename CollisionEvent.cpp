#include "CollisionEvent.h"

CollisionEvent::CollisionEvent(int object1ID, int object2ID, Timeline* timeline)
    : Event(COLLISION, 1, timeline), object1ID(object1ID), object2ID(object2ID) {}

int CollisionEvent::getObject1ID() const {
    return object1ID;
}

int CollisionEvent::getObject2ID() const {
    return object2ID;
}
