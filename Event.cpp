#include "Event.h"

Event::Event(const EventType& type, int priority)
    : type(type), priority(priority), timestamp(std::chrono::steady_clock::now()) {}

EventType Event::getType() const {
    return type;
}

int Event::getPriority() const {
    return priority;
}

std::chrono::steady_clock::time_point Event::getTimestamp() const {
    return timestamp;
}
