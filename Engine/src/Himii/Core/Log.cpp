#include "Log.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

namespace Himii
{
    Ref<spdlog::logger> Log::s_CoreLogger;
    Ref<spdlog::logger> Log::s_ClientLogger;

    std::string GetFileName(const char *fullPath)
    {
        std::string pathStr(fullPath);
        size_t lastSlash = pathStr.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            return pathStr.substr(lastSlash + 1);
        }
        return pathStr;
    }

    void Log::Init(bool toFile, const std::string &filePath)
    {
        std::vector<spdlog::sink_ptr> sinks;

        // 控制台输出
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] [%n] [%l] %v%$");
        sinks.push_back(console_sink);

        // 文件输出（如果需要）
        if (toFile)
        {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, true);
            file_sink->set_pattern("[%T] [%n] [%l] %v");
            sinks.push_back(file_sink);
        }

        // 创建loggers
        s_CoreLogger = std::make_shared<spdlog::logger>("CORE", sinks.begin(), sinks.end());
        s_ClientLogger = std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());

        // 设置日志级别
        s_CoreLogger->set_level(spdlog::level::trace);
        s_ClientLogger->set_level(spdlog::level::trace);

        // 注册loggers
        spdlog::register_logger(s_CoreLogger);
        spdlog::register_logger(s_ClientLogger);
    }

    spdlog::level::level_enum Log::ConvertLogLevel(LogLevel level)
    {
        switch (level)
        {
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

        spdlog::level::level_enum spdLevel = ConvertLogLevel(level);

        // 选择合适的logger
        if (level == LogLevel::Core_Info || level == LogLevel::Core_Warning || level == LogLevel::Core_Error)
        {
            s_CoreLogger->log(spdLevel, fullMessage);
        }
        else
        {
            s_ClientLogger->log(spdLevel, fullMessage);
        }
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

        // 使用错误级别记录断言失败
        if (s_CoreLogger)
        {
            s_CoreLogger->critical(assertMessage);
        }
        else
        {
            // 如果日志系统未初始化，直接输出到控制台
            std::cerr << "[CRITICAL] " << assertMessage << std::endl;
        }

        // 刷新所有日志
        if (s_CoreLogger)
            s_CoreLogger->flush();
        if (s_ClientLogger)
            s_ClientLogger->flush();

        // 在调试模式下触发断点
#ifdef HIMII_DEBUG
#ifdef _WIN32
        __debugbreak();
#else
        __builtin_trap();
#endif
#endif

        // 终止程序
        std::abort();
    }
} // namespace Himii
