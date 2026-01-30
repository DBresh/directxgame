#pragma once
#include <functional>
#include <atomic>

namespace dx3d
{
    // Helper struct passed to Dispatch tasks
    struct JobDispatchArgs
    {
        uint32_t jobIndex;  // The global index of the first item in this batch
        uint32_t groupIndex; // Which "chunk" this is (0, 1, 2...)
    };

    namespace JobSystem
    {
        // Call this once at engine startup
        void Initialize();

        // Add a single task to the queue
        void Execute(const std::function<void()>& job);

        // Automatically split a large loop into parallel chunks
        // itemCount: Total number of items (e.g., 2500 cubes)
        // groupSize: How many items per thread (e.g., 250). Optimization parameter.
        // task: The function to call. Receives (jobIndex, count)
        void Dispatch(uint32_t itemCount, uint32_t groupSize, const std::function<void(class JobDispatchArgs)>& task);

        // Check if any threads are currently working
        bool IsBusy();

        // Wait until all current jobs are finished.
        // The calling thread will HELP execute jobs while waiting (prevents idleness).
        void Wait();

        void Shutdown();
    }
}