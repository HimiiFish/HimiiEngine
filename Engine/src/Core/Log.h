#pragma once
#include "Hepch.h"

namespace Core
{
    enum class LogLevel {
        Info,
        Warning,
        Error
    };

    class Log {
    public:
        static void Init(bool toFile = false, const std::string &filePath = "log.txt");

        static void Print(LogLevel level, const std::string &message, const char *file, const char *function, int line);
    };

}

#define LOG_INFO(msg)::Core::Log::Print(LogLevel::Info, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_WARNING(msg)::Core::Log::Print(LogLevel::Warning, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR(msg)::Core::Log::Print(LogLevel::Error, msg, __FILE__, __FUNCTION__, __LINE__)
