#include <DX3D/Core/Logger.h>

namespace dx3d {

    Logger::Logger() = default;

    Logger::~Logger()
    {
        if (m_logFile.is_open())
            m_logFile.close();
    }

    Logger& Logger::getInstance()
    {
        static Logger instance;
        return instance;
    }

    void Logger::setLogToFile(const std::string& filename)
    {
        std::lock_guard lock(m_mutex);
        if (m_logFile.is_open())
            m_logFile.close();

        m_logFile.open(filename, std::ios::out | std::ios::app);
    }

    void Logger::log(LogLevel level, const std::string& message, const std::string& category)
    {
        if (level > m_logLevel)
            return;

        std::lock_guard lock(m_mutex);

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
        localtime_s(&tm, &time_t);

        std::string levelStr;
        switch (level) {
        case LogLevel::Error:   levelStr = "ERROR"; break;
        case LogLevel::Warning: levelStr = "WARN";  break;
        case LogLevel::Info:    levelStr = "INFO";  break;
        case LogLevel::Debug:   levelStr = "DEBUG"; break;
        }

        std::string prefix = std::format("[{}][{:02}:{:02}:{:02}]{}{}",
            levelStr, tm.tm_hour, tm.tm_min, tm.tm_sec,
            category.empty() ? "" : "[" + category + "] ",
            message);

        writeToOutput(prefix, level);
    }

    void Logger::writeToOutput(const std::string& formattedMessage, LogLevel level)
    {
        if (m_logToConsole)
        {
            if (m_useColor)
            {
                switch (level)
                {
                case LogLevel::Error:   std::cout << "\x1B[31m"; break;
                case LogLevel::Warning: std::cout << "\x1B[33m"; break;
                case LogLevel::Info:    std::cout << "\x1B[37m"; break;
                case LogLevel::Debug:   std::cout << "\x1B[90m"; break;
                }
            }

            std::cout << formattedMessage << std::endl;
            if (m_useColor)
                std::cout << "\x1B[0m";
        }

        if (m_logFile.is_open())
        {
            m_logFile << formattedMessage << std::endl;
            m_logFile.flush();
        }
    }
}