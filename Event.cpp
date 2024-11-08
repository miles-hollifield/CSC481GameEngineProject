#include "Event.h"

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