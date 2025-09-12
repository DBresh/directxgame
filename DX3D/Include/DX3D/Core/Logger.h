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



#define DX3DLogError(message)\
		getLogger().log((Logger::LogLevel::Error), message);\

#define DX3DLogWarning(message)\
		getLogger().log((Logger::LogLevel::Warning), message);\

#define DX3DLogInfo(message)\
		getLogger().log((Logger::LogLevel::Info), message);\

#define DX3DLogError(message)\
		getLogger().log((Logger::LogLevel::Error), message);\

#define DX3DLogErrorAndThrow(message)\
	{\
		DX3DLogError(message);\
		throw std::runtime_error(message);\
	}
}

