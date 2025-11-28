#include "Log.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>
#include <algorithm>
#include <iostream>

namespace Himii
{
    Ref<spdlog::logger> Log::s_CoreLogger;
    Ref<spdlog::logger> Log::s_ClientLogger;

    std::string Log::GetFileName(const char *fullPath)
    {
        if (!fullPath) return {};
        const char *slash = std::max(strrchr(fullPath, '/'), strrchr(fullPath, '\\'));
        return slash ? std::string(slash + 1) : std::string(fullPath);
    }

    void Log::Init(bool toFile, const std::string &filePath)
    {
        std::vector<spdlog::sink_ptr> sinks;
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] [%n] [%l] %v%$");
        sinks.push_back(console_sink);

        if (toFile)
        {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, true);
            file_sink->set_pattern("[%T] [%n] [%l] %v");
            sinks.push_back(file_sink);
        }

        s_CoreLogger = std::make_shared<spdlog::logger>("CORE", sinks.begin(), sinks.end());
        s_ClientLogger = std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());

        s_CoreLogger->set_level(spdlog::level::trace);
        s_ClientLogger->set_level(spdlog::level::trace);

        spdlog::register_logger(s_CoreLogger);
        spdlog::register_logger(s_ClientLogger);
    }

    spdlog::level::level_enum Log::ConvertLogLevel(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace:
            case LogLevel::Core_Trace:
                return spdlog::level::trace;
            case LogLevel::Info:
            case LogLevel::Core_Info:
                return spdlog::level::info;
            case LogLevel::Warning:
            case LogLevel::Core_Warning:
                return spdlog::level::warn;
            case LogLevel::Error:
            case LogLevel::Core_Error:
                return spdlog::level::err;
            default:
                return spdlog::level::info;
        }
    }

    void Log::Print(LogLevel level, const std::string &message, const char *file, const char *function, int line)
    {
        std::string fileName = GetFileName(file);
        std::string fullMessage = fmt::format("[{}:{} {}] {}", fileName, line, function, message);

        auto &logger = (level == LogLevel::Core_Info || level == LogLevel::Core_Warning || level == LogLevel::Core_Error)
            ? s_CoreLogger
            : s_ClientLogger;
        logger->log(ConvertLogLevel(level), fullMessage);
    }

    void Log::Assert(bool condition, const std::string &message, const char *file, const char *function, int line)
    {
        if (!condition)
        {
            HandleAssertFailure(message, file, function, line);
        }
    }

    void Log::HandleAssertFailure(const std::string &message, const char *file, const char *function, int line)
    {
        std::string fileName = GetFileName(file);
        std::string assertMessage = fmt::format("断言失败: {} [{}:{} in {}]", message, fileName, line, function);

        if (s_CoreLogger)
            s_CoreLogger->critical(assertMessage);
        else
            std::cerr << "[CRITICAL] " << assertMessage << std::endl;

        if (s_CoreLogger) s_CoreLogger->flush();
        if (s_ClientLogger) s_ClientLogger->flush();

#ifdef HIMII_DEBUG
#ifdef _WIN32
        __debugbreak();
#else
        __builtin_trap();
#endif
#endif
        std::abort();
    }
} // namespace Himii
