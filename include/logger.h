#pragma once


#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
	
#define LOG_LEVEL_DEBUG spdlog::level::debug
#define LOG_LEVEL_INFO spdlog::level::info
#define LOG_LEVEL_WARN spdlog::level::warn
#define LOG_LEVEL_ERROR spdlog::level::err

#define LOG_SET_LOGLEVEL(...) {     \
    if (!logger) {                  \
        INIT_LOGGER();              \
    }                               \
    logger->set_level(__VA_ARGS__); \
}

#define INIT_LOGGER() {	\
    if (!logger) {	\
		spdlog::set_pattern("[%T] %^[%l]%$ %v%"); \
		logger = spdlog::stdout_color_mt("client"); \
	} \
}

#define LOG_WARN(...)	  { INIT_LOGGER(); logger->warn(__VA_ARGS__);		}
#define LOG_DEBUG(...)	  { INIT_LOGGER(); logger->debug(__VA_ARGS__);		}
#define LOG_INFO(...)	  { INIT_LOGGER(); logger->info(__VA_ARGS__);		}
#define LOG_ERROR(...)	  { INIT_LOGGER(); logger->error(__VA_ARGS__);		}


extern std::shared_ptr<spdlog::logger> logger;
