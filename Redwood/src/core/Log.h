#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "Core.h"

namespace rwd {

	class RWD_API Log {
	public:
		static void Init();
	private:
		inline static Ref<spdlog::logger> sConsole;
	};

}

