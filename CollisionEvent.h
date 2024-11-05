#ifndef COLLISION_EVENT_H
#define COLLISION_EVENT_H

#include "Event.h"

/**
 * @brief Represents a collision event in the game.
 */
class CollisionEvent : public Event {
public:
    CollisionEvent(int object1ID, int object2ID);

    int getObject1ID() const;
    int getObject2ID() const;

private:
    int object1ID; // ID of the first object involved in the collision
    int object2ID; // ID of the second object involved in the collision
};

#endif // COLLISION_EVENT_H
