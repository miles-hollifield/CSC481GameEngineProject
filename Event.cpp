#include "Event.h"
#include <nlohmann/json.hpp> // JSON library for serialization

Event::Event(const EventType& type, int priority, Timeline* timeline, std::unordered_map<std::string, int> data)
    : type(type), priority(priority), data(std::move(data)) {
    timestamp = timeline->getTime();
}

EventType Event::getType() const {
    return type;
}

int Event::getPriority() const {
    return priority;
}

int64_t Event::getTimestamp() const {
    return timestamp;
}

const std::unordered_map<std::string, int>& Event::getData() const {
    return data;
}

// Serialize the event to JSON
std::string Event::serialize() const {
    nlohmann::json jsonData;
    jsonData["type"] = static_cast<int>(type);
    jsonData["priority"] = priority;
    jsonData["timestamp"] = timestamp;
    jsonData["data"] = data;
    return jsonData.dump();
}

// Deserialize an event from JSON
Event Event::deserialize(const std::string& jsonString) {
    nlohmann::json jsonData = nlohmann::json::parse(jsonString);
    EventType type = static_cast<EventType>(jsonData["type"].get<int>());
    int priority = jsonData["priority"].get<int>();
    int64_t timestamp = jsonData["timestamp"].get<int64_t>();
    std::unordered_map<std::string, int> data = jsonData["data"].get<std::unordered_map<std::string, int>>();

    // Assuming we have access to a global or static timeline pointer for timestamp retrieval.
    Timeline* timeline = nullptr; // Replace this with the actual timeline instance
    Event event(type, priority, timeline, data);
    event.timestamp = timestamp; // Set the timestamp directly for deserialized events
    return event;
}
