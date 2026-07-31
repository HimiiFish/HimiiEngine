#include "Hepch.h"
#include "EngineCore/Core/JobSystem.h"

namespace Himii
{
    std::mutex JobSystem::s_WorkerMutex;
    std::condition_variable JobSystem::s_WorkerCondition;
    std::queue<std::function<void()>> JobSystem::s_WorkerTasks;
    std::vector<JobSystem::WorkerState> JobSystem::s_Workers;
    std::atomic<bool> JobSystem::s_Running{false};
    std::atomic<uint32_t> JobSystem::s_PendingWorkerTasks{0};

    std::mutex JobSystem::s_MainThreadMutex;
    std::queue<std::function<void()>> JobSystem::s_MainThreadCompletions;
    std::atomic<uint32_t> JobSystem::s_PendingMainThreadCompletions{0};

    void JobSystem::Initialize(uint32_t workerCount)
    {
        if (s_Running.load())
            return;

        if (workerCount == 0)
        {
            const unsigned hardwareConcurrency = std::thread::hardware_concurrency();
            workerCount = hardwareConcurrency == 0 ? 2u : std::max(2u, hardwareConcurrency / 2u);
        }

        s_Running.store(true);
        s_Workers.resize(workerCount);
        for (uint32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex)
            s_Workers[workerIndex].Thread = std::thread(&JobSystem::WorkerLoop, workerIndex);
    }

    void JobSystem::Shutdown()
    {
        if (!s_Running.load())
            return;

        {
            std::lock_guard<std::mutex> lock(s_WorkerMutex);
            s_Running.store(false);
        }
        s_WorkerCondition.notify_all();

        for (WorkerState &worker : s_Workers)
        {
            if (worker.Thread.joinable())
                worker.Thread.join();
        }
        s_Workers.clear();

        {
            std::lock_guard<std::mutex> lock(s_WorkerMutex);
            while (!s_WorkerTasks.empty())
                s_WorkerTasks.pop();
        }
        {
            std::lock_guard<std::mutex> lock(s_MainThreadMutex);
            while (!s_MainThreadCompletions.empty())
                s_MainThreadCompletions.pop();
        }
        s_PendingWorkerTasks.store(0);
        s_PendingMainThreadCompletions.store(0);
    }

    bool JobSystem::IsInitialized()
    {
        return s_Running.load();
    }

    void JobSystem::Submit(std::function<void()> workerTask)
    {
        Submit(std::move(workerTask), nullptr);
    }

    void JobSystem::Submit(
            std::function<void()> workerTask, std::function<void()> mainThreadCompletion)
    {
        if (!workerTask)
            return;

        if (!s_Running.load())
        {
            // JobSystem 未启动时退化为同步执行，保证阶段一/二仍可用。
            workerTask();
            if (mainThreadCompletion)
                mainThreadCompletion();
            return;
        }

        auto wrappedTask = [task = std::move(workerTask),
                            completion = std::move(mainThreadCompletion)]() mutable
        {
            task();
            if (completion)
            {
                {
                    std::lock_guard<std::mutex> lock(s_MainThreadMutex);
                    s_MainThreadCompletions.push(std::move(completion));
                }
                s_PendingMainThreadCompletions.fetch_add(1);
            }
            s_PendingWorkerTasks.fetch_sub(1);
        };

        {
            std::lock_guard<std::mutex> lock(s_WorkerMutex);
            s_WorkerTasks.push(std::move(wrappedTask));
        }
        s_PendingWorkerTasks.fetch_add(1);
        s_WorkerCondition.notify_one();
    }

    void JobSystem::PumpMainThreadCompletions()
    {
        for (;;)
        {
            std::function<void()> completion;
            {
                std::lock_guard<std::mutex> lock(s_MainThreadMutex);
                if (s_MainThreadCompletions.empty())
                    break;
                completion = std::move(s_MainThreadCompletions.front());
                s_MainThreadCompletions.pop();
            }
            if (completion)
                completion();
            s_PendingMainThreadCompletions.fetch_sub(1);
        }
    }

    uint32_t JobSystem::GetPendingWorkerTaskCount()
    {
        return s_PendingWorkerTasks.load();
    }

    uint32_t JobSystem::GetPendingMainThreadCompletionCount()
    {
        return s_PendingMainThreadCompletions.load();
    }

    void JobSystem::WorkerLoop(uint32_t)
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(s_WorkerMutex);
                s_WorkerCondition.wait(lock, []()
                {
                    return !s_Running.load() || !s_WorkerTasks.empty();
                });
                if (!s_Running.load() && s_WorkerTasks.empty())
                    return;
                task = std::move(s_WorkerTasks.front());
                s_WorkerTasks.pop();
            }
            if (task)
                task();
        }
    }
}
