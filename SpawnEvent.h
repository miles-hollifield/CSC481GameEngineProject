#ifndef SPAWN_EVENT_H
#define SPAWN_EVENT_H

#include "Event.h"

/**
 * @brief Structure to represent spawn event data.
 *
 * Contains the coordinates (`spawnX` and `spawnY`) where the object should spawn.
 */
struct SpawnEventData {
    int spawnX; // X-coordinate for the spawn location.
    int spawnY; // Y-coordinate for the spawn location.
};

/**
 * @brief Represents a spawn event in the game.
 *
 * This event is triggered when an object, such as a player or item, spawns within
 * the game world. It holds information on the object involved in the event.
 */
class SpawnEvent : public Event {
public:
    /**
     * @brief Constructs a SpawnEvent with the specified object ID.
     *
     * @param objectID The ID of the object that will spawn.
     * @param timeline Pointer to the timeline used for timestamping the event.
     */
    SpawnEvent(int objectID, Timeline* timeline);

    /**
     * @brief Retrieves the ID of the object associated with this spawn event.
     *
     * @return The object ID for the spawn event.
     */
    int getObjectID() const;

private:
    int objectID; // ID of the object that triggered the spawn event.
};

#endif // SPAWN_EVENT_H
