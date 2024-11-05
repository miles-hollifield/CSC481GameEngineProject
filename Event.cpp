#include "Event.h"

Event::Event(const EventType& type, int priority, Timeline* timeline)
    : type(type), priority(priority) {
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
