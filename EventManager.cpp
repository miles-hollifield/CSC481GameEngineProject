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

// Serialize an event to JSON
nlohmann::json EventManager::serializeEvent(const std::shared_ptr<Event>& event) {
    nlohmann::json jsonEvent;
    jsonEvent["type"] = event->getType();
    jsonEvent["priority"] = event->getPriority();
    jsonEvent["timestamp"] = event->getTimestamp();
    // Add additional event data if necessary
    return jsonEvent;
}

// Deserialize an event from JSON
std::shared_ptr<Event> EventManager::deserializeEvent(const nlohmann::json& jsonEvent) {
    EventType type = static_cast<EventType>(jsonEvent["type"].get<int>());
    int priority = jsonEvent["priority"];
    int64_t timestamp = jsonEvent["timestamp"];

    // For this example, assume all events use a default Timeline pointer (could be enhanced for specific timelines)
    auto event = std::make_shared<Event>(type, priority, nullptr);
    return event;
}

// Send a serialized event over a ZeroMQ socket
void EventManager::sendEvent(const std::shared_ptr<Event>& event, zmq::socket_t& socket) {
    nlohmann::json jsonEvent = serializeEvent(event);
    std::string serializedEvent = jsonEvent.dump();
    zmq::message_t message(serializedEvent.begin(), serializedEvent.end());
    socket.send(message, zmq::send_flags::none);
}

// Receive an event from a ZeroMQ socket and reconstruct it
void EventManager::receiveEvent(zmq::socket_t& socket) {
    zmq::message_t message;
    if (socket.recv(message, zmq::recv_flags::dontwait)) {
        std::string serializedEvent(static_cast<char*>(message.data()), message.size());
        nlohmann::json jsonEvent = nlohmann::json::parse(serializedEvent);
        auto event = deserializeEvent(jsonEvent);
        raiseEvent(event); // Raise received event to be processed locally
    }
}
