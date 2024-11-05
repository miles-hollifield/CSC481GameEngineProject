#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <chrono>

/**
 * @brief Base class for all game events.
 */
class Event {
public:
    Event(const std::string& type, int priority);
    virtual ~Event() = default;

    std::string getType() const;
    int getPriority() const;
    std::chrono::steady_clock::time_point getTimestamp() const;

private:
    std::string type; // Type of the event (e.g., "Collision", "Death")
    int priority;     // Priority level of the event
    std::chrono::steady_clock::time_point timestamp; // Timestamp of the event
};

#endif // EVENT_H
