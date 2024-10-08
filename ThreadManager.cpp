#include "ThreadManager.h"

// Constructor
ThreadManager::ThreadManager() {
    // Initialize any resources needed for thread management
}

// Destructor: Ensures all threads are joined before destruction
ThreadManager::~ThreadManager() {
    joinAll();
}

// Method to create and store a new thread
void ThreadManager::createThread(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(threadMutex);  // Ensure thread-safe access
    threads.emplace_back(std::thread(task));  // Create and store the thread
}

// Method to join all threads
void ThreadManager::joinAll() {
    std::lock_guard<std::mutex> lock(threadMutex);  // Ensure thread-safe access
    for (std::thread& t : threads) {
        if (t.joinable()) {
            t.join();  // Wait for the thread to finish
        }
    }
    threads.clear();  // Clear the vector after joining all threads
}
