#include "pch.h"
#include "glad/glad.h"
#include "Shader.h"

namespace rwd {

	Shader::Shader() {
		static u32 curShaderId = 0;
		mShaderId = curShaderId++;
	}

	bool Shader::operator==(const Shader& other) const {
		return mShaderId == other.mShaderId;
	}

	u32 Shader::Id() const {
		return mShaderId;
	}

}
