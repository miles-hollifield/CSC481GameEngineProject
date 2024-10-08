#include "Timeline.h"

// Define the target ticks per second (e.g., 60 FPS)
const float Timeline::targetTPS = 60.0f;

Timeline::Timeline(Timeline* anchor, float tic)
    : tic(tic), paused(false), elapsedPausedTime(std::chrono::seconds(0)) {
    // Initialize the timeline with the start time
    startTime = std::chrono::system_clock::now();  // Record the start time
}

// Get the current time relative to the anchor
int64_t Timeline::getTime() const {
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsedTime;

    // Calculate elapsed time based on whether the timeline is paused or running
    if (paused) {
        // If paused, compute elapsed time up to lastPausedTime
        elapsedTime = lastPausedTime - startTime - elapsedPausedTime;
    }
    else {
        // If not paused, compute elapsed time up to now
        elapsedTime = now - startTime - elapsedPausedTime;
    }

    elapsedTime /= tic;  // Adjust the time by the current tic rate

    // Return the elapsed time in milliseconds
    return static_cast<int64_t>(elapsedTime.count() * 1000.0f);
}

// Pause the timeline
void Timeline::pause() {
    if (!paused) {
        // Set paused state and record the time when the timeline was paused
        paused = true;
        lastPausedTime = std::chrono::system_clock::now();
    }
}

// Unpause the timeline
void Timeline::unpause() {
    if (paused) {
        // Calculate the time since it was paused and update the paused time
        auto now = std::chrono::system_clock::now();
        elapsedPausedTime += now - lastPausedTime;  // Add paused duration to total paused time
        paused = false;  // Reset the paused state
    }
}

// Check if the timeline is paused
bool Timeline::isPaused() const {
    return paused;
}

// Change the tic rate
void Timeline::changeTic(float newTic) {
    tic = newTic;  // Set the new tic rate for the timeline
}

// Get the current tic rate
float Timeline::getTic() const {
    return tic;  // Return the current tic rate
}

// Getter for target ticks per second (for use in frame delta calculation)
float Timeline::getTargetTPS() {
    return targetTPS;  // Return the target ticks per second (60 FPS)
}
