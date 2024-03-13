#pragma once
#include "pch.h"
#include "core/Core.h"

namespace rwd {

	class System {
	public:
		static void ReadFile(const std::string& filepath, std::vector<u8>& contents);
	};
}

