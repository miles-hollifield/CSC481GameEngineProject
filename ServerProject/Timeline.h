#pragma once
#include <chrono>
#include <mutex>

/**
 * Timeline class represents a flexible time system that can track real or game time, handle
 * time scaling, and support features such as pausing, unpausing, and time anchoring. It is thread-safe
 * for multithreaded environments.
 */
class Timeline {
private:
    std::chrono::time_point<std::chrono::system_clock> startTime;  // The time point when the timeline started
    std::chrono::time_point<std::chrono::system_clock> lastPausedTime;  // The time point when the timeline was last paused
    std::chrono::duration<float> elapsedPausedTime;  // Accumulated duration of time spent paused

    float tic;  // The number of anchor timeline units per step, allowing time scaling
    bool paused;  // Indicates whether the timeline is currently paused

    std::mutex m;  // Mutex to ensure thread-safe updates to the timeline state

    static const float targetTPS;  // Target ticks per second, representing how often updates should occur

public:
    /**
     * Constructor for the Timeline class. Initializes a timeline object with optional
     * anchoring and a customizable tic rate.
     *
     * @param anchor A pointer to another timeline that this timeline can be anchored to (optional)
     * @param tic The number of units per step, controlling the rate of time scaling (default is 1.0 for real-time)
     */
    Timeline(Timeline* anchor = nullptr, float tic = 1.0f);

    /**
     * Get the current time relative to the timeline's start. Accounts for pausing and time scaling.
     *
     * @return The current time in milliseconds, adjusted for elapsed time and pause duration
     */
    int64_t getTime() const;

    /**
     * Pauses the timeline, freezing time. All objects depending on this timeline will stop moving
     * or updating until the timeline is unpaused.
     */
    void pause();

    /**
     * Unpauses the timeline, resuming time flow from where it was paused. Objects depending on this timeline
     * will resume their movements and updates.
     */
    void unpause();

    /**
     * Checks whether the timeline is currently paused.
     *
     * @return True if the timeline is paused, false otherwise
     */
    bool isPaused() const;

    /**
     * Changes the tic rate (time scaling) of the timeline. Adjusting this allows the timeline to run
     * faster or slower than real-time.
     *
     * @param newTic The new tic rate
     */
    void changeTic(float newTic);

    /**
     * Retrieves the current tic rate of the timeline.
     *
     * @return The current tic rate controlling time scaling
     */
    float getTic() const;

    /**
     * Retrieves the target ticks per second (TPS) for the game loop.
     *
     * @return The target TPS
     */
    static float getTargetTPS();
};
