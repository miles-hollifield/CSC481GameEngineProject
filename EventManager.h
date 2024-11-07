#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <zmq.hpp>
#include "EventQueue.h"
#include "Event.h"

/**
 * @brief The EventManager class for managing event listeners, raising, dispatching, and networked events.
 */
class EventManager {
public:
    EventManager() = default;
    ~EventManager() = default;

    // Singleton instance accessor
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    // Method to register a handler for a specific event type
    void registerHandler(EventType eventType, std::function<void(std::shared_ptr<Event>)> handler);

    // Method to raise an event
    void raiseEvent(std::shared_ptr<Event> event);

    // Method to dispatch all queued events to their respective handlers
    void dispatchEvents();

private:
    // Delete copy constructor and assignment operator
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    EventQueue eventQueue; // Queue to manage events
    std::unordered_map<EventType, std::vector<std::function<void(std::shared_ptr<Event>)>>> handlers; // Map for event handlers
};

#endif // EVENT_MANAGER_H
