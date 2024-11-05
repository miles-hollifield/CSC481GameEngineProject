#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>

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
    Event(const EventType& type, int priority);
    virtual ~Event() = default;

    EventType getType() const;
    int getPriority() const;
    std::chrono::steady_clock::time_point getTimestamp() const;

private:
    EventType type; // Type of the event (e.g., "Collision", "Death")
    int priority;     // Priority level of the event
    std::chrono::steady_clock::time_point timestamp; // Timestamp of the event
};

#endif // EVENT_H
