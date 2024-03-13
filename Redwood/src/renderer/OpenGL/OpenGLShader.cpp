#include "pch.h"
#include "glad/glad.h"
#include "OpenGLShader.h"

namespace rwd {

	OpenGLShader::OpenGLShader(const std::string& vertFile, const std::string& fragFile) {
		CreateShader(vertFile, fragFile);
	}

	void OpenGLShader::Bind() const {
		glUseProgram(mGLShaderId);
	}

	inline int OpenGLShader::GetUniformLocation(const std::string& name) {
		if (!mUniformLocations.contains(name)) {
			i32 location = glGetUniformLocation(mGLShaderId, name.c_str());
			mUniformLocations[name] = location;
			return location;
		}

		return mUniformLocations[name];
	}

	void OpenGLShader::CreateShader(const std::string& vertFile, const std::string& fragFile) {
		const std::string vertCodeString = LoadShaderFile(vertFile);
		const std::string fragCodeString = LoadShaderFile(fragFile);

		const char* vertCode = vertCodeString.c_str();
		const char* fragCode = fragCodeString.c_str();

		// Vertex shader
		const i32 vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vertCode, NULL);
		glCompileShader(vertex);

#if _DEBUG
		CheckCompileErrors(vertex, "Vertex");
#endif

		// Fragment Shader
		const i32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fragCode, NULL);
		glCompileShader(fragment);

#if _DEBUG
		CheckCompileErrors(fragment, "Fragment");
#endif

		// Shader Program
		mGLShaderId = glCreateProgram();
		glAttachShader(mGLShaderId, vertex);
		glAttachShader(mGLShaderId, fragment);
		glLinkProgram(mGLShaderId);

#if _DEBUG
		CheckCompileErrors(mGLShaderId, "Shader Linking");
#endif

		// Delete the shaders as they're linked into our program and are no longer necessary
		glDeleteShader(vertex);
		glDeleteShader(fragment);

		Bind();
	}

	// Loads a shader file into a string
	std::string OpenGLShader::LoadShaderFile(const std::string& filePath) const {
		std::fstream file(filePath);
		std::string codeString;

		if (file.fail()) {
			std::cout << "Failed to open " << filePath << std::endl;
			return codeString;
		}

		std::stringstream stream;
		stream << file.rdbuf();
		codeString = stream.str();

		file.close();
		return codeString;
	}

	// Checks for compilation errors in the shaders
	void OpenGLShader::CheckCompileErrors(u32 shader, const std::string& type) const {
		i32 success;
		char infoLog[1024];

		if (type == "Shader Linking") {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);

			if (success) return;

			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "Error within " << type << std::endl;
			std::cout << infoLog << std::endl;
			return;
		}

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

		if (success) return;

		glGetShaderInfoLog(shader, 1024, NULL, infoLog);
		std::cout << "Error within " << type << std::endl;
		std::cout << infoLog << std::endl;
	}

}
