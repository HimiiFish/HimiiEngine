#pragma once
#include "Hepch.h"

namespace Engine
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
#define LOG_CORE_INFO(msg) ::Engine::Log::Print(::Engine::LogLevel::Core_Info, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_CORE_WARNING(msg) ::Engine::Log::Print(::Engine::LogLevel::Core_Warning, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_CORE_ERROR(msg) ::Engine::Log::Print(::Engine::LogLevel::Core_Error, msg, __FILE__, __FUNCTION__, __LINE__)

//Client
#define LOG_INFO(msg) Engine::Log::Print(::Engine::LogLevel::Info, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_WARNING(msg) ::Engine::Log::Print(::Engine::LogLevel::Warning, msg, __FILE__, __FUNCTION__, __LINE__)
#define LOG_ERROR(msg) ::Engine::Log::Print(::Engine::LogLevel::Error, msg, __FILE__, __FUNCTION__, __LINE__)
