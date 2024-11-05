#ifndef SPAWN_EVENT_H
#define SPAWN_EVENT_H

#include "Event.h"

/**
 * @brief Represents a spawn event in the game.
 */
class SpawnEvent : public Event {
public:
    SpawnEvent(int objectID, Timeline* timeline);

    int getObjectID() const;

private:
    int objectID; // ID of the object that triggered the spawn event
};

#endif // SPAWN_EVENT_H
#pragma once
