#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace rose
{
	class Logger
	{
	public:

		Logger(Logger& other) = delete;

		void operator=(const Logger&) = delete;

		static void Initialize();

		static std::shared_ptr<spdlog::logger>& GetLogger();

		static void SetLevel(spdlog::level::level_enum level);

	private:

		Logger() {};

		~Logger() {};
	
		static std::shared_ptr<spdlog::logger> logger;

	};
}

// We can exclude our logging on release builds by using conditional compilation
#define LOG_TRACE(...)	rose::Logger::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...)	rose::Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)	rose::Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)	rose::Logger::GetLogger()->error(__VA_ARGS__)

#ifdef ROSE_RELEASE_BUILD
#define LOG_TRACE
#define LOG_INFO
#define LOG_WARN
#define LOG_ERROR
#endif

