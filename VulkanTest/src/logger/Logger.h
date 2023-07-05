#pragma once

#include <cstdio>
#include <cstdint>
#include <cassert>
#include <inttypes.h>
#include "Core.h"

namespace vkEngine
{
	enum class LogLevel : uint8_t
	{
		Trace,
		Debug,
		Info,
		Warn,
		Error,
		Fatal
	};

	class Logger
	{
	public:

		template <typename... Args>
		void _log(LogLevel level, const char* message, Args... args)
		{
			assert(level >= LogLevel::Trace && level <= LogLevel::Fatal && "Invalid log level");
			assert(message);


			constexpr size_t BUFFER_SIZE = KB(4);
			char buffer[BUFFER_SIZE] = {};
			sprintf_s(buffer, sizeof(buffer), message, args...);

			setConsoleColor(level);
			if (level == LogLevel::Info)
				printf_s("%s: %s\n", m_LogLevelNames[static_cast<uint8_t>(level)], buffer);
			else
				printf_s("%s: %s\n\n", m_LogLevelNames[static_cast<uint8_t>(level)], buffer);

			setConsoleColor(LogLevel::Debug);
		}
		static Logger& getInstance();

	private:
		Logger() = default;
		~Logger() = default;

		void setConsoleColor(LogLevel level);
	private:
		static Logger s_Instance;

		const char* m_LogLevelNames[6] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

	};
}

#define ENGINE_TRACE(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Trace,msg, __VA_ARGS__)
#define ENGINE_DEBUG(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Debug, msg, __VA_ARGS__)
#define ENGINE_INFO(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Info, msg, __VA_ARGS__)
#define ENGINE_WARN(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Warn, msg, __VA_ARGS__)
#define ENGINE_ERROR(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Error, msg, __VA_ARGS__)
#define ENGINE_FATAL(msg, ...) vkEngine::Logger::getInstance()._log(vkEngine::LogLevel::Fatal, msg, __VA_ARGS__)

#ifdef _DEBUG
#define ENGINE_ASSERT(x, msg, ...)					\
	{													\
		if(!(x))										\
		{												\
			ENGINE_ERROR(msg, __VA_ARGS__);				\
			__debugbreak();							 	\
		}												\
	}												
#elif NDEBUG 
#define ENGINE_ASSERT(x, msg, ...) x;
#endif

