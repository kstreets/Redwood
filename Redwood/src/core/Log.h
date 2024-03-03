#pragma once
#include "spdlog/spdlog.h"
#include "Core.h"

namespace rwd {

	class RWD_API Log {
	public:
		static void Init() {
			spdlog::set_pattern("[%s:%#] %v%$");
		}
	};

#if !RWD_PRODUCTION

	// Set spdlog level to critical (max) so all logs get included
	//#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL

	// In order to have print file name, line number, etc, we need to use
	// the spdlog macros since they rely on compile time macros

	#define RWD_LOG(...)       SPDLOG_TRACE(__VA_ARGS__)
	#define RWD_LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
	#define RWD_LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
	#define RWD_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
	#define RWD_LOG_CRIT(...)  SPDLOG_CRITICAL(__VA_ARGS__)

#else

	#define RWD_LOG(...)       0
	#define RWD_LOG_INFO(...)  0
	#define RWD_LOG_WARN(...)  0
	#define RWD_LOG_ERROR(...) 0
	#define RWD_LOG_CRIT(...)  0

#endif

}
