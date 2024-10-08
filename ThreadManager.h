#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <thread>
#include <vector>
#include <functional>
#include <mutex>

/**
 * ThreadManager is responsible for managing a collection of threads,
 * ensuring safe creation, execution, and cleanup of threads in a multithreaded environment.
 */
class ThreadManager {
public:
    /**
     * Constructor for the ThreadManager class.
     * Initializes the thread manager, setting up the structures needed to handle multiple threads.
     */
    ThreadManager();

    /**
     * Destructor for the ThreadManager class.
     * Ensures that all threads are properly joined and cleaned up before the object is destroyed.
     */
    ~ThreadManager();

    /**
     * Creates and starts a new thread to execute the given task.
     *
     * @param task A function object (std::function<void()>) that represents the work
     *             to be executed in the new thread.
     */
    void createThread(std::function<void()> task);

    /**
     * Joins all running threads, ensuring that they complete their execution
     * before the program exits. This method is typically called at the end of the program
     * to prevent premature termination of threads.
     */
    void joinAll();

private:
    std::vector<std::thread> threads;  // Container for managing multiple threads.
    std::mutex threadMutex;  // Mutex for ensuring thread-safe operations when creating and joining threads.
};

#endif // THREAD_MANAGER_H
