#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <format>
#include <iostream>
#include <chrono>
#include <iomanip>

namespace dx3d {
    class Logger {
    public:
        enum class LogLevel { Error = 0, Warning, Info, Debug };

        static Logger& getInstance();

        void setLogLevel(LogLevel level) noexcept { m_logLevel = level; }
        void setLogToFile(const std::string& filename);
        void setLogToConsole(bool enable) noexcept { m_logToConsole = enable; }
        void enableColor(bool enable) noexcept { m_useColor = enable; }

        void log(LogLevel level, const std::string& message, const std::string& category = {});
        void error(const std::string& message, const std::string& cat = {}) { log(LogLevel::Error, message, cat); }
        void warning(const std::string& message, const std::string& cat = {}) { log(LogLevel::Warning, message, cat); }
        void info(const std::string& message, const std::string& cat = {}) { log(LogLevel::Info, message, cat); }
        void debug(const std::string& message, const std::string& cat = {}) { log(LogLevel::Debug, message, cat); }

    private:
        Logger();
        ~Logger();
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        void writeToOutput(const std::string& formattedMessage, LogLevel level);

    private:
        LogLevel m_logLevel = LogLevel::Info;
        bool m_logToConsole = true;
        bool m_useColor = true;
        std::ofstream m_logFile;
        std::mutex m_mutex;
    };

    template<typename... Args>
    inline std::string formatLog(const std::format_string<Args...> fmt, Args&&... args)
    {
        return std::format(fmt, std::forward<Args>(args)...);
    }

}

#define DX3D_LOG_ERROR(fmt, ...)   dx3d::Logger::getInstance().error(dx3d::formatLog(fmt, ##__VA_ARGS__))
#define DX3D_LOG_WARNING(fmt, ...) dx3d::Logger::getInstance().warning(dx3d::formatLog(fmt, ##__VA_ARGS__))
#define DX3D_LOG_INFO(fmt, ...)    dx3d::Logger::getInstance().info(dx3d::formatLog(fmt, ##__VA_ARGS__))
#define DX3D_LOG_DEBUG(fmt, ...)   dx3d::Logger::getInstance().debug(dx3d::formatLog(fmt, ##__VA_ARGS__))

#define DX3D_LOG_THROW(exception, fmt, ...) \
    do { \
        std::string message = dx3d::formatLog(fmt, ##__VA_ARGS__); \
        DX3D_LOG_ERROR("{}", message); \
        throw exception(message); \
    } while(0)

#define DX3D_LOG_THROW_ERROR(fmt, ...) DX3D_LOG_THROW(std::runtime_error, fmt, ##__VA_ARGS__)
