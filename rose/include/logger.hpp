#ifndef ROSE_INCLUDE_LOGGER
#define ROSE_INCLUDE_LOGGER

#include <memory>

#include <spdlog/spdlog.h>

namespace rose {

class Logger {
  public:
    Logger(Logger& other) = delete;

    void operator=(const Logger&) = delete;

    static void init();

    static std::shared_ptr<spdlog::logger>& get_logger();

    static void set_level(spdlog::level::level_enum level);

  private:
    Logger(){};

    ~Logger(){};

    static std::shared_ptr<spdlog::logger> logger;
};

} // namespace rose

#define LOG_TRACE(...) rose::Logger::get_logger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) rose::Logger::get_logger()->debug(__VA_ARGS__)
#define LOG_INFO(...) rose::Logger::get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...) rose::Logger::get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) rose::Logger::get_logger()->error(__VA_ARGS__)

// Strip logging on release builds
#ifdef ROSE_RELEASE_BUILD
#define LOG_TRACE(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#endif

#endif
