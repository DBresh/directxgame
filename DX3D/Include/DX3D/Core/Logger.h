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

		explicit Logger(LogLevel logLevel = LogLevel::Error);

		void log(LogLevel logLevel, const char* message) const;
	private:
		LogLevel m_logLevel = LogLevel::Error;
	};
}

