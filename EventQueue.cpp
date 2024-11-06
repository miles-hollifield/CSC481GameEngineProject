#include "EventQueue.h"

// Method to push a new event onto the queue
void EventQueue::pushEvent(std::shared_ptr<Event> event) {
    eventQueue.push(event);
}

// Method to pop an event from the queue
std::shared_ptr<Event> EventQueue::popEvent() {
    if (eventQueue.empty()) {
        return nullptr;
    }
    auto event = eventQueue.front();
    eventQueue.pop();
    return event;
}

// Method to check if the event queue is empty
bool EventQueue::isEmpty() const {
    return eventQueue.empty();
}
