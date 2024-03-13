#pragma once
#include <string>
#include <unordered_map>
#include "core/Core.h"

namespace rwd {

	class Shader {
	public:
		Shader();
		bool operator==(const Shader& other) const;
		u32 Id() const;
	private:
		u32 mShaderId;
	};

}

