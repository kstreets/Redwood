#pragma once
#include <string>
#include <unordered_map>
#include "core/Core.h"

namespace rwd {

	class Shader {
	public:
		Shader() = default;
		Shader(const std::string& vertFile, const std::string& fragFile);

		bool operator==(const Shader& other) const;

		u32 Id() const;
		void Bind() const;
	private:
		inline i32 GetUniformLocation(const std::string& name);
		void CreateShader(const std::string& vertFile, const std::string& fragFile);
		std::string LoadShaderFile(const std::string& filePath) const;
		void CheckCompileErrors(u32 shader, const std::string& type) const;

		i32 mGLShaderId;
		u32 mShaderId;
		std::unordered_map<std::string, i32> mUniformLocations;
	};

}

