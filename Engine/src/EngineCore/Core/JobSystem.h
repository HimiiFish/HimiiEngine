#pragma once

#include "EngineCore/Core/Core.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>

namespace Himii
{
    class JobSystem
    {
    public:
        static void Initialize(uint32_t workerCount = 0);
        static void Shutdown();
        static bool IsInitialized();

        // 在工作线程执行；完成后可选择投递到主线程队列。
        static void Submit(std::function<void()> workerTask);
        static void Submit(
                std::function<void()> workerTask, std::function<void()> mainThreadCompletion);

        // 必须在主线程每帧调用，处理完成回调（含 OpenGL 上传）。
        static void PumpMainThreadCompletions();

        static uint32_t GetPendingWorkerTaskCount();
        static uint32_t GetPendingMainThreadCompletionCount();

    private:
        struct WorkerState
        {
            std::thread Thread;
        };

        static void WorkerLoop(uint32_t workerIndex);

        static std::mutex s_WorkerMutex;
        static std::condition_variable s_WorkerCondition;
        static std::queue<std::function<void()>> s_WorkerTasks;
        static std::vector<WorkerState> s_Workers;
        static std::atomic<bool> s_Running;
        static std::atomic<uint32_t> s_PendingWorkerTasks;

        static std::mutex s_MainThreadMutex;
        static std::queue<std::function<void()>> s_MainThreadCompletions;
        static std::atomic<uint32_t> s_PendingMainThreadCompletions;
    };
}
