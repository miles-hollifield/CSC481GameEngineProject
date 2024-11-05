#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <memory>
#include "Event.h"  // Assuming Event is defined elsewhere

/**
 * @brief Comparator for priority queue to order events based on priority and timestamp.
 */
struct EventCompare {
    bool operator()(const std::shared_ptr<Event>& lhs, const std::shared_ptr<Event>& rhs) const {
        if (lhs->getPriority() == rhs->getPriority()) {
            // Earlier timestamps have higher priority
            return lhs->getTimestamp() > rhs->getTimestamp();
        }
        // Higher priority number means higher priority
        return lhs->getPriority() < rhs->getPriority();
    }
};

/**
 * @brief The EventQueue class for managing a queue of events.
 */
class EventQueue {
public:
    // Method to push a new event onto the queue
    void pushEvent(std::shared_ptr<Event> event);

    // Method to pop an event from the queue
    std::shared_ptr<Event> popEvent();

    // Method to check if the event queue is empty
    bool isEmpty() const;

private:
    std::queue<std::shared_ptr<Event>> eventQueue;  // The queue that holds events
};

#endif // EVENT_QUEUE_H
