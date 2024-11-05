#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <string>
#include "EventQueue.h"
#include "Event.h" 

/**
 * @brief Enumeration of event types.
 */
enum EventType {
    PLAYER_MOVED,
    PLATFORM_COLLISION,
    // Add more event types as needed
};

/**
 * @brief The EventManager class for managing event listeners, raising, and dispatching events.
 */
class EventManager {
public:
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
    // Private constructor and destructor to prevent instantiation
    EventManager() = default;
    ~EventManager() = default;

    // Delete copy constructor and assignment operator
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    // Queue to manage events
    EventQueue eventQueue;

    // Map to store event handlers for each event type
    std::unordered_map<EventType, std::vector<std::function<void(std::shared_ptr<Event>)>>> handlers;
};

#endif // EVENT_MANAGER_H
