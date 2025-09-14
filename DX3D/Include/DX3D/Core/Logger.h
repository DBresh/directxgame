#pragma once





namespace dx3d
{

	class Logger
	{
	public:

		enum class LogLevel
		{
			Error = 0,
			Warning,
			Info
		};

		~Logger();
		explicit Logger(LogLevel logLevel = LogLevel::Error);
		void log(LogLevel logLevel, const char* message);

	protected:
		Logger(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator = (const Logger&) = delete;
		Logger& operator = (Logger&&) = delete;
		
	private:
		LogLevel m_logLevel = LogLevel::Error;
	};

}

#define DX3DLog(logger, type, message)\
logger.log((type), message);

#define DX3DLogThrow(logger, exception, type, message)\
{\
DX3DLog(logger, type, message);\
throw exception(message);\
}
