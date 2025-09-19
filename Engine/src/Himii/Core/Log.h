#pragma once
#include "Hepch.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "Himii/Core/Core.h"

namespace Himii
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

        template<typename... Args>
        static void PrintFormatted(LogLevel level, const char *file, const char *function, int line,
                                   const std::string &format, Args &&...args)
        {
            std::string message = fmt::format(format, std::forward<Args>(args)...);
            Print(level, message, file, function, line);
        }

        static void Assert(bool condition, const std::string &message, const char *file, const char *function,
                           int line);

        template<typename... Args>
        static void AssertFormatted(bool condition, const char *file, const char *function, int line,
                                    const std::string &format, Args &&...args)
        {
            if (!condition)
            {
                std::string message = fmt::format(format, std::forward<Args>(args)...);
                Assert(condition, message, file, function, line);
            }
        }

        static Ref<spdlog::logger> &GetCoreLogger()
        {
            return s_CoreLogger;
        }
        
        static Ref<spdlog::logger> &GetClientLogger()
        {
            return s_ClientLogger;
        }


    private:
        static Ref<spdlog::logger> s_CoreLogger;

        static Ref<spdlog::logger> s_ClientLogger;

        static spdlog::level::level_enum ConvertLogLevel(LogLevel level);

        static void HandleAssertFailure(const std::string &message, const char *file, const char *function, int line);
    };

} // namespace Himii

// Core
#define HIMII_CORE_INFO(fmt, ...)                                                                                      \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Core_Info, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define HIMII_CORE_WARNING(fmt, ...)                                                                                   \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Core_Warning, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define HIMII_CORE_ERROR(fmt, ...)                                                                                     \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Core_Error, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

// Client
#define HIMII_INFO(fmt, ...)                                                                                           \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Info, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define HIMII_WARNING(fmt, ...)                                                                                        \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Warning, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define HIMII_ERROR(fmt, ...)                                                                                          \
    ::Himii::Log::PrintFormatted(::Himii::LogLevel::Error, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef HIMII_DEBUG
#define HIMII_ASSERT(condition, msg)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::Himii::Log::Assert(false, msg, __FILE__, __FUNCTION__, __LINE__);                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

#define HIMII_ASSERT_F(condition, fmt, ...)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::Himii::Log::AssertFormatted(false, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);                \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

#define HIMII_CORE_ASSERT(condition, msg)                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            LOG_CORE_ERROR(fmt::format("断言失败: {}", msg));                                                          \
            ::Himii::Log::Assert(false, msg, __FILE__, __FUNCTION__, __LINE__);                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

#define HIMII_CORE_ASSERT_F(condition, fmt, ...)                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            std::string msg = fmt::format(fmt, ##__VA_ARGS__);                                                         \
            LOG_CORE_ERROR(fmt::format("断言失败: {}", msg));                                                          \
            ::Himii::Log::AssertFormatted(false, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);                \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)
#else
#define HIMII_ASSERT(condition, fmt, ...) ((void)0)
#define HIMII_CORE_ASSERT(condition, fmt, ...) ((void)0)
#endif

#define HIMII_VERIFY(condition, msg)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::Himii::Log::Assert(false, msg, __FILE__, __FUNCTION__, __LINE__);                                        \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)

#define HIMII_VERIFY_F(condition, fmt, ...)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(condition))                                                                                              \
        {                                                                                                              \
            ::Himii::Log::AssertFormatted(false, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__);                \
        }                                                                                                              \
    }                                                                                                                  \
    while (0)
