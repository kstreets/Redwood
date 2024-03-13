#pragma once
#include "pch.h"
#include "core/Core.h"
#include "renderer/Shader.h"

namespace rwd {

	class OpenGLShader : public Shader {
	public:
		OpenGLShader() = default;
		OpenGLShader(const std::string& vertFile, const std::string& fragFile);
		void Bind() const;
	private:
		inline i32 GetUniformLocation(const std::string& name);
		void CreateShader(const std::string& vertFile, const std::string& fragFile);
		std::string LoadShaderFile(const std::string& filePath) const;
		void CheckCompileErrors(u32 shader, const std::string& type) const;

		i32 mGLShaderId;
		std::unordered_map<std::string, i32> mUniformLocations;
	};

}
