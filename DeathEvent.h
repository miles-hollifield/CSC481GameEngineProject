#ifndef DEATH_EVENT_H
#define DEATH_EVENT_H

#include "Event.h"

/**
 * @brief Represents a death event in the game.
 *
 * A DeathEvent is triggered when an object in the game "dies" or reaches a condition
 * that signifies it should be removed or reset, such as falling out of bounds.
 * It stores the ID of the object involved, allowing the game to handle the
 * consequences of this event (e.g., respawning or resetting the object).
 */
class DeathEvent : public Event {
public:
    /**
     * @brief Constructs a new DeathEvent.
     *
     * @param objectID The ID of the object that triggered the death event.
     * @param timeline Pointer to the game's timeline (for event timestamping).
     */
    DeathEvent(int objectID, Timeline* timeline);

    /**
     * @brief Gets the ID of the object that triggered the death event.
     *
     * @return int The ID of the object.
     */
    int getObjectID() const;

private:
    int objectID; // ID of the object that triggered the death event.
};

#endif // DEATH_EVENT_H
