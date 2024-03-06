#pragma once

// Define the logging level before including spdlog header
#if !RWD_PRODUCTION
	#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
	#define LEVEL_ENUM spdlog::level::trace
#endif

#include "spdlog/spdlog.h"
#include "Core.h"

namespace rwd {

	class RWD_API Log {
	public:
		static void Init() {
			spdlog::set_pattern("[%s:%#] %v%$");
			spdlog::set_level(LEVEL_ENUM);
		}
	};

#if !RWD_PRODUCTION

	// In order to have print file name, line number, etc, we need to use
	// the spdlog macros since they rely on compile time macros

	#define RWD_LOG(...)       SPDLOG_TRACE(__VA_ARGS__)
	#define RWD_LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
	#define RWD_LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
	#define RWD_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
	#define RWD_LOG_CRIT(...)  SPDLOG_CRITICAL(__VA_ARGS__); std::exit(0)

	#define RWD_ASSERT(expression, ...) do { if (!expression) { SPDLOG_CRITICAL(__VA_ARGS__); assert(expression); }} while(false)

#else

	#define RWD_LOG(...)       0
	#define RWD_LOG_INFO(...)  0
	#define RWD_LOG_WARN(...)  0
	#define RWD_LOG_ERROR(...) 0
	#define RWD_LOG_CRIT(...)  0

#endif

}
