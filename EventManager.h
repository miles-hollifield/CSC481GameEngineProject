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
 * @brief The EventManager class for managing event listeners, raising events, dispatching events, and handling networked events.
 *
 * The EventManager class is responsible for managing the entire event-handling system. It allows
 * for the registration of handlers for specific event types, raising new events, and dispatching
 * queued events to the appropriate handlers. As a singleton, this class ensures there is a
 * centralized instance that can be accessed globally within the application.
 */
class EventManager {
public:
    /**
     * @brief Default constructor for EventManager.
     *
     * Initializes the event manager without any specific setup. Since EventManager is a singleton,
     * this constructor is only called internally.
     */
    EventManager() = default;

    /**
     * @brief Default destructor for EventManager.
     *
     * Cleans up resources used by the EventManager, if any.
     */
    ~EventManager() = default;

    /**
     * @brief Singleton instance accessor for EventManager.
     *
     * This function provides access to the singleton instance of EventManager. Ensures there is
     * only one instance of EventManager in the application.
     *
     * @return EventManager& Reference to the singleton instance of EventManager.
     */
    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    /**
     * @brief Registers a handler for a specific event type.
     *
     * This method associates a callback function with a specified event type. When an event of
     * that type is dispatched, the registered handler will be invoked.
     *
     * @param eventType The type of event to register the handler for.
     * @param handler A function to handle the event, accepting a shared pointer to the Event object.
     */
    void registerHandler(EventType eventType, std::function<void(std::shared_ptr<Event>)> handler);

    /**
     * @brief Raises an event by adding it to the event queue.
     *
     * Adds an event to the EventManager’s event queue, where it will be dispatched during the
     * dispatching process.
     *
     * @param event A shared pointer to the Event object being raised.
     */
    void raiseEvent(std::shared_ptr<Event> event);

    /**
     * @brief Dispatches all queued events to their respective handlers.
     *
     * Iterates through the event queue and dispatches each event to its registered handlers. This
     * process invokes the callbacks associated with each event type, allowing them to be handled
     * accordingly.
     */
    void dispatchEvents();

private:
    /**
     * @brief Deleted copy constructor to enforce singleton pattern.
     */
    EventManager(const EventManager&) = delete;

    /**
     * @brief Deleted assignment operator to enforce singleton pattern.
     */
    EventManager& operator=(const EventManager&) = delete;

    EventQueue eventQueue; // Queue to manage events, allowing them to be dispatched in sequence.

    /// Map storing event handlers for each event type. Each event type can have multiple handlers.
    std::unordered_map<EventType, std::vector<std::function<void(std::shared_ptr<Event>)>>> handlers;
};

#endif // EVENT_MANAGER_H
