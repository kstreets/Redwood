#include "pch.h"
#include "glad/glad.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"
#include "OpenGLBuffer.h"
#include "OpenGLRenderer.h"

namespace rwd { 

	void OpenGLRenderer::DrawMesh(Mesh& mesh, Shader& shader) {
// 		shader.Bind();
// 		mesh.mGLVertexArray->Bind();
// 		glDrawElements(GL_TRIANGLES, mesh.mIndexCount, GL_UNSIGNED_INT, 0);
	}

	void OpenGLRenderer::SetClearColor() {
		
	}

	void OpenGLRenderer::Clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

}
