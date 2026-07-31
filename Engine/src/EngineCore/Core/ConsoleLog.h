#pragma once

#include "EngineCore/Core/Log.h"

#include <mutex>
#include <string>
#include <vector>

namespace Himii
{
    struct ConsoleLogEntry
    {
        LogLevel Level = LogLevel::Info;
        std::string Message;
        std::string Source;
        float Timestamp = 0.0f;
    };

    class ConsoleLog
    {
    public:
        static constexpr size_t MaxEntries = 2000;

        static void Push(LogLevel level, const std::string& message, const std::string& source);
        static std::vector<ConsoleLogEntry> GetEntries();
        static void Clear();

    private:
        static std::vector<ConsoleLogEntry> s_Entries;
        static std::mutex s_Mutex;
    };
}
