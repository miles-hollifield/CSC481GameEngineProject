#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <memory>
#include "Event.h"  // Assuming Event is defined elsewhere

/**
 * @brief Comparator for the priority queue to order events based on priority and timestamp.
 *
 * The EventCompare struct is a custom comparator used for sorting events in the priority queue.
 * Events with a higher priority value are prioritized, and if two events have the same priority,
 * the event with the earlier timestamp is prioritized.
 */
struct EventCompare {
    /**
     * @brief Comparison operator for event prioritization.
     *
     * @param lhs The left-hand side event to compare.
     * @param rhs The right-hand side event to compare.
     * @return True if lhs has lower priority or a later timestamp than rhs, false otherwise.
     */
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
 *
 * This class is responsible for managing events in a queue, allowing them to be added (pushed),
 * removed (popped), and checked for emptiness. Events are prioritized based on their priority
 * and timestamp, with higher priority events processed first.
 */
class EventQueue {
public:
    /**
     * @brief Pushes a new event onto the queue.
     *
     * Adds an event to the queue, where it will be prioritized based on the EventCompare
     * comparator. Events with higher priority or earlier timestamps are placed ahead.
     *
     * @param event The event to be added to the queue.
     */
    void pushEvent(std::shared_ptr<Event> event);

    /**
     * @brief Pops an event from the queue.
     *
     * Removes the highest-priority event from the queue and returns it.
     *
     * @return A shared pointer to the highest-priority event, or nullptr if the queue is empty.
     */
    std::shared_ptr<Event> popEvent();

    /**
     * @brief Checks if the event queue is empty.
     *
     * This method allows checking whether the queue has any remaining events.
     *
     * @return True if the queue is empty, false otherwise.
     */
    bool isEmpty() const;

private:
    std::queue<std::shared_ptr<Event>> eventQueue;  // Queue that holds events in order of priority.
};

#endif // EVENT_QUEUE_H
