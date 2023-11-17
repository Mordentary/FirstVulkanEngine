#include "pch.h"

#include "Logger.h"
#include <Windows.h>

namespace vkEngine
{
	Logger Logger::s_Instance{};

	Logger& Logger::getInstance()
	{
		return s_Instance;
	}

	void Logger::setConsoleColor(LogLevel level)
	{
		static HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		uint32_t color = 0;
		switch (level)
		{
		case LogLevel::Trace:
			color = FOREGROUND_INTENSITY;
			break;
		case LogLevel::Debug:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		case LogLevel::Info:
			color = FOREGROUND_GREEN;
			break;
		case LogLevel::Warn:
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case LogLevel::Error:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case LogLevel::Fatal:
			color = FOREGROUND_RED;
			break;
		}

		SetConsoleTextAttribute(consoleHandle, color);
	}
}