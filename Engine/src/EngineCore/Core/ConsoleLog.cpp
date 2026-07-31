#include "Hepch.h"
#include "ConsoleLog.h"

#include <chrono>

namespace Himii
{
    std::vector<ConsoleLogEntry> ConsoleLog::s_Entries;
    std::mutex ConsoleLog::s_Mutex;

    static float GetTimestamp()
    {
        using clock = std::chrono::steady_clock;
        static const auto start = clock::now();
        const auto now = clock::now();
        return std::chrono::duration<float>(now - start).count();
    }

    void ConsoleLog::Push(LogLevel level, const std::string& message, const std::string& source)
    {
        std::lock_guard<std::mutex> lock(s_Mutex);

        if (s_Entries.size() >= MaxEntries)
            s_Entries.erase(s_Entries.begin());

        s_Entries.push_back({level, message, source, GetTimestamp()});
    }

    std::vector<ConsoleLogEntry> ConsoleLog::GetEntries()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        return s_Entries;
    }

    void ConsoleLog::Clear()
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Entries.clear();
    }
}
