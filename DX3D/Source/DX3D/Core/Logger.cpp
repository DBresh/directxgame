#include <DX3D/Core/Logger.h>
#include <iostream>




dx3d::Logger::~Logger()
{
}

dx3d::Logger::Logger(LogLevel logLevel) : m_logLevel(logLevel)
{
	std::clog << "D3D1 Game Engine Tests" << "\n";
	std::clog << "======================" << "\n";
}

void dx3d::Logger::log(LogLevel level, const char* message)
{
	auto logLevelToString = [](LogLevel level) {
		switch (level)
		{
		case LogLevel::Info: return "Info";
		case LogLevel::Error: return "Error";
		case LogLevel::Warning: return "Warning";
		default: return "Unkown";
		}
	};

	if (level > m_logLevel) return;
	std::clog << "[DX3D " << logLevelToString(level) << "]: " << message << "\n";
}
