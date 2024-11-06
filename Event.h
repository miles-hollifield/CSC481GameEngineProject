#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>
#include "Timeline.h"

/**
 * @brief Enumeration of event types.
 */
enum EventType {
    COLLISION,
    SPAWN,
    DEATH,
    INPUT
    // Add more event types as needed
};

/**
 * @brief Base class for all game events.
 */
class Event {
public:
    Event(const EventType& type, int priority, Timeline* timeline);
    virtual ~Event() = default;

    EventType getType() const;
    int getPriority() const;
    int64_t getTimestamp() const;

private:
    EventType type; // Type of the event (e.g., "Collision", "Death")
    int priority;     // Priority level of the event
    int64_t timestamp; // Timestamp of the event in milliseconds
};

#endif // EVENT_H
