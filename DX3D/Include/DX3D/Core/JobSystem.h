#pragma once
#include <functional>
#include <atomic>

namespace dx3d
{
    struct JobDispatchArgs
    {
        uint32_t jobIndex;
        uint32_t groupIndex;
    };

    namespace JobSystem
    {
        void Initialize();
        void Execute(const std::function<void()>& job);
        void Dispatch(uint32_t itemCount, uint32_t groupSize, const std::function<void(class JobDispatchArgs)>& task);
        bool IsBusy();
        void Wait();

        void Shutdown();
    }
}