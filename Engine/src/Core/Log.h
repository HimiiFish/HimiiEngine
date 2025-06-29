#pragma once
#include "Hepch.h"

namespace Core
{
    enum class LogLevel {
        Info,
        Warning,
        Error,
        Core_Info,
        Core_Warning,
        Core_Error
    };

    class Log {
    public:
        static void Init(bool toFile = false, const std::string &filePath = "log.txt");

        static void Print(LogLevel level, const std::string &message, const char *file, const char *function, int line);
    };

}

//Core
#define LOG_CORE_INFO(msg) ::Core::Log::Print(::Core::LogLevel::Core_Info, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_CORE_WARNING(msg) ::Core::Log::Print(::Core::LogLevel::Core_Warning, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_CORE_ERROR(msg) ::Core::Log::Print(::Core::LogLevel::Core_Error, msg, __FILE__, __FUNCTION__, __LINE__)

//Client
#define LOG_INFO(msg) ::Core::Log::Print(::Core::LogLevel::Info, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_WARNING(msg) ::Core::Log::Print(::Core::LogLevel::Warning, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR(msg) ::Core::Log::Print(::Core::LogLevel::Error, msg, __FILE__, __FUNCTION__, __LINE__)
