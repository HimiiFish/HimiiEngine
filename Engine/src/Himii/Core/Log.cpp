#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace Himii
{
    std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

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
        if (toFile) {
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
}