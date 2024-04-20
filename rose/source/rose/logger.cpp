#include <rose/logger.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

namespace rose {

std::shared_ptr<spdlog::logger> Logger::logger;

void Logger::init() {
    logger = spdlog::stdout_color_mt("ROSE", spdlog::color_mode::automatic);
    logger->set_pattern("[%r] %l:: %v");
    logger->set_level(spdlog::level::trace);
}

void Logger::set_level(spdlog::level::level_enum level) { logger->set_level(level); }

std::shared_ptr<spdlog::logger>& Logger::get_logger() { return logger; }

} // namespace rose
