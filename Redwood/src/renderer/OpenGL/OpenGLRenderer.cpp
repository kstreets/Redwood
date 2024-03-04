#include "pch.h"
#include "glad/glad.h"
#include "OpenGLRenderer.h"

namespace rwd { 

	void OpenGLRenderer::SetClearColor() {
		
	}

	void OpenGLRenderer::Clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	}

}
