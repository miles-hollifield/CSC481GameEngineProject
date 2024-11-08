#ifndef COLLISION_EVENT_H
#define COLLISION_EVENT_H

#include "Event.h"

/**
 * @brief Represents a collision event in the game.
 *
 * A CollisionEvent is generated whenever two game objects collide.
 * It stores the IDs of the two objects involved in the collision,
 * allowing the game to handle collision resolution between them.
 */
class CollisionEvent : public Event {
public:
    /**
     * @brief Constructs a new CollisionEvent.
     *
     * @param object1ID The ID of the first object involved in the collision.
     * @param object2ID The ID of the second object involved in the collision.
     * @param timeline Pointer to the game's timeline (for event timestamping).
     */
    CollisionEvent(int object1ID, int object2ID, Timeline* timeline);

    /**
     * @brief Gets the ID of the first object involved in the collision.
     *
     * @return int The ID of the first object.
     */
    int getObject1ID() const;

    /**
     * @brief Gets the ID of the second object involved in the collision.
     *
     * @return int The ID of the second object.
     */
    int getObject2ID() const;

private:
    int object1ID; // ID of the first object involved in the collision.
    int object2ID; // ID of the second object involved in the collision.
};

#endif // COLLISION_EVENT_H
