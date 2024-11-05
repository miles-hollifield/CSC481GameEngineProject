#ifndef DEATH_EVENT_H
#define DEATH_EVENT_H

#include "Event.h"

/**
 * @brief Represents a death event in the game.
 */
class DeathEvent : public Event {
public:
    DeathEvent(int objectID);

    int getObjectID() const;

private:
    int objectID; // ID of the object that triggered the death event
};

#endif // DEATH_EVENT_H
#pragma once
