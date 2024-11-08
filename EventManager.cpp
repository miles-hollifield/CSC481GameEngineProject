#include "EventManager.h"
#include <iostream> // For debug output

// Method to register a handler for a specific event type
void EventManager::registerHandler(EventType eventType, std::function<void(std::shared_ptr<Event>)> handler) {
    handlers[eventType].push_back(handler);
}

// Method to raise an event
void EventManager::raiseEvent(std::shared_ptr<Event> event) {
    eventQueue.pushEvent(event);
}

// Method to dispatch all queued events to their respective handlers
void EventManager::dispatchEvents() {
    while (!eventQueue.isEmpty()) {
        std::shared_ptr<Event> event = eventQueue.popEvent();
        EventType eventType = event->getType();

        // Dispatch the event to all registered handlers for its type
        if (handlers.find(eventType) != handlers.end()) {
            for (const auto& handler : handlers[eventType]) {
                handler(event);
            }
        }
        else {
            std::cerr << "No handlers registered for event type: " << static_cast<int>(eventType) << std::endl;
        }
    }
}