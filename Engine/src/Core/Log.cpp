#include "Log.h"
#include "Hepch.h"

namespace Core
{
    bool logToFile = false;
    std::string logFilePath = "log.txt";

    std::string GetTimestamp()
    {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
        return std::string("[") + buf + "]";
    }

    std::string LevelToString(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Info:
                return "INFO";
            case LogLevel::Warning:
                return "WARN";
            case LogLevel::Error:
                return "ERROR";
            default:
                return "UNKNOWN";
        }
    }

    std::string GetColorCode(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Info:
                return "\033[32m"; // Green
            case LogLevel::Warning:
                return "\033[33m"; // Yellow
            case LogLevel::Error:
                return "\033[31m"; // Red
            default:
                return "\033[0m";
        }
    }

    void OutputToConsole(LogLevel level, const std::string &fullMsg)
    {
        std::cout << GetColorCode(level) << fullMsg << "\033[0m" << std::endl;
    }

    void OutputToFile(const std::string &fullMsg)
    {
        std::ofstream ofs(logFilePath, std::ios::app);
        if (ofs)
        {
            ofs << fullMsg << std::endl;
        }
    }


    void Log::Init(bool toFile, const std::string &filePath)
    {
        std::cout << "Log system initialized." << std::endl;
        logToFile = toFile;
        logFilePath = filePath;
        if (logToFile)
        {
            std::ofstream ofs(logFilePath, std::ios::trunc);
            ofs << "==== Log Start ====\n";
        }
    }

    void Log::Print(LogLevel level, const std::string &message, const char *file, const char *function, int line)
    {
        std::ostringstream oss;
        oss << GetTimestamp() << " [" << LevelToString(level) << "] "
            << "[" << file << ":" << line << " " << function << "] " << message;

        std::string fullMsg = oss.str();

        OutputToConsole(level, fullMsg);
        if (logToFile)
            OutputToFile(fullMsg);
    }
} // namespace Core
