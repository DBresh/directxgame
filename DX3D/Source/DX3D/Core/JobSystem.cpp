#include <DX3D/Core/JobSystem.h>
#include <DX3D/Core/Logger.h> // Assuming you have a logger, otherwise use printf

#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace dx3d::JobSystem
{
    // --- Internal State ---
    uint32_t numThreads = 0;
    std::deque<std::function<void()>> jobQueue;
    std::vector<std::thread> workers;

    std::mutex queueMutex;
    std::condition_variable wakeCondition;
    std::atomic<bool> shutdown{ false };

    // Label to track unfinished work
    std::atomic<uint64_t> currentLabel{ 0 };
    std::atomic<uint64_t> finishedLabel{ 0 };

    // Function executed by each worker thread
    void WorkerLoop()
    {
        while (true)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(queueMutex);

                // Wait until there is a job or we are shutting down
                wakeCondition.wait(lock, [] {
                    return !jobQueue.empty() || shutdown.load();
                    });

                if (shutdown.load() && jobQueue.empty())
                    return;

                // Steal work from the front
                job = std::move(jobQueue.front());
                jobQueue.pop_front();
            }

            // Execute the job
            job();

            // Mark one unit of work as done
            finishedLabel.fetch_add(1);
        }
    }

    void Initialize()
    {
        // Detect CPU cores. Leave one for the main thread? Usually better to use all.
        // If you have 8 cores, hardware_concurrency returns 8.
        numThreads = std::thread::hardware_concurrency();

        // Safety check for weird hardware
        if (numThreads == 0) numThreads = 1;

        // Create workers
        DX3D_LOG_INFO("JobSystem: Spawning {} worker threads.", numThreads);

        for (uint32_t i = 0; i < numThreads; ++i)
        {
            workers.emplace_back(WorkerLoop);
        }
    }

    void Execute(const std::function<void()>& job)
    {
        currentLabel.fetch_add(1); // Increment "To-Do" counter

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            jobQueue.push_back(job);
        }

        wakeCondition.notify_one(); // Wake up one worker
    }

    void Dispatch(uint32_t itemCount, uint32_t groupSize, const std::function<void(JobDispatchArgs)>& task)
    {
        if (itemCount == 0 || groupSize == 0) return;

        // Calculate how many chunks we need
        uint32_t groupCount = (itemCount + groupSize - 1) / groupSize;

        for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
        {
            // Calculate the starting index for this chunk
            uint32_t jobIndex = groupIndex * groupSize;

            // Queue the chunk as a job
            Execute([jobIndex, groupIndex, task]() {
                JobDispatchArgs args;
                args.jobIndex = jobIndex;
                args.groupIndex = groupIndex;
                task(args);
                });
        }
    }

    bool IsBusy()
    {
        // If "Finished" count < "To-Do" count, we are busy.
        return finishedLabel.load() < currentLabel.load();
    }

    void Wait()
    {
        // Active Waiting:
        // While the queue is not empty or workers are working, 
        // the Main Thread should grab jobs and help out!
        while (IsBusy())
        {
            std::function<void()> job;
            bool foundJob = false;

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!jobQueue.empty())
                {
                    job = std::move(jobQueue.front());
                    jobQueue.pop_front();
                    foundJob = true;
                }
            }

            if (foundJob)
            {
                // Help execute
                job();
                finishedLabel.fetch_add(1);
            }
            else
            {
                // If queue is empty but threads are still processing,
                // we yield so we don't burn CPU spinning on "IsBusy"
                std::this_thread::yield();
            }
        }
    }

    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            shutdown.store(true);
        }

        // Wake up all threads so they can see the shutdown flag
        wakeCondition.notify_all();

        // Wait for every thread to finish its current job and exit
        for (std::thread& worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        workers.clear();
        DX3D_LOG_INFO("JobSystem: Shutdown complete.");
    }
}