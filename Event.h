#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>
#include <unordered_map>
#include "Timeline.h"

/**
 * @brief Enumeration of event types.
 *
 * EventType is used to specify different types of events that can occur in the game.
 * Examples include COLLISION, SPAWN, DEATH, and INPUT, which represent specific actions
 * or occurrences within the game world.
 */
enum EventType {
    COLLISION,  // Event triggered when two objects collide.
    SPAWN,      // Event triggered when an object is spawned.
    DEATH,      // Event triggered when an object dies.
    INPUT       // Event triggered in response to player input.
};

/**
 * @brief Base class for all game events.
 *
 * The Event class serves as a foundational structure for all types of events within the game.
 * It includes basic properties, such as the event type, priority, timestamp, and additional data.
 * Derived event types inherit from this class and may add more specific attributes or methods.
 */
class Event {
public:
    /**
     * @brief Constructs a new Event.
     *
     * @param type The type of the event (e.g., COLLISION, DEATH).
     * @param priority The priority level of the event (used to determine event handling order).
     * @param timeline Pointer to the game's timeline (for generating the event timestamp).
     * @param data Additional data related to the event, represented as key-value pairs (optional).
     */
    Event(const EventType& type, int priority, Timeline* timeline, std::unordered_map<std::string, int> data = {});

    /**
     * @brief Virtual destructor for the Event class.
     *
     * Allows for proper cleanup of derived event classes.
     */
    virtual ~Event() = default;

    /**
     * @brief Gets the type of the event.
     *
     * @return EventType The type of the event.
     */
    EventType getType() const;

    /**
     * @brief Gets the priority of the event.
     *
     * @return int The priority level of the event.
     */
    int getPriority() const;

    /**
     * @brief Gets the timestamp of the event in milliseconds.
     *
     * The timestamp is generated at the moment the event is created, based on the game's timeline.
     *
     * @return int64_t The timestamp of the event in milliseconds.
     */
    int64_t getTimestamp() const;

    /**
     * @brief Gets additional data associated with the event.
     *
     * @return const std::unordered_map<std::string, int>& A reference to the data map.
     */
    const std::unordered_map<std::string, int>& getData() const;

private:
    EventType type; // The type of the event (e.g., COLLISION, DEATH).
    int priority;   // Priority level of the event.
    int64_t timestamp; // Timestamp of the event in milliseconds.
    std::unordered_map<std::string, int> data; // Additional data associated with the event.
};

#endif // EVENT_H
