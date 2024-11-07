#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>
#include <unordered_map>
#include <nlohmann/json.hpp> // For JSON serialization
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
    Event(const EventType& type, int priority, Timeline* timeline, std::unordered_map<std::string, int> data = {});
    virtual ~Event() = default;

    EventType getType() const;
    int getPriority() const;
    int64_t getTimestamp() const;
    const std::unordered_map<std::string, int>& getData() const;

    // Serialize the event to JSON
    std::string serialize() const;

    // Deserialize an event from JSON
    static Event deserialize(const std::string& jsonString);

private:
    EventType type; // Type of the event (e.g., "Collision", "Death")
    int priority;   // Priority level of the event
    int64_t timestamp; // Timestamp of the event in milliseconds
    std::unordered_map<std::string, int> data; // Additional event data
};

#endif // EVENT_H
