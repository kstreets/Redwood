#include "Log.h"

namespace rwd {

	void Log::Init() {
		spdlog::set_pattern("%^[%T] %v%$");
		spdlog::set_level(spdlog::level::trace);
		sConsole = spdlog::stderr_color_mt("Console");
	}

}
