#pragma once
#include "renderer/Renderer.h"

namespace rwd {

	class OpenGLRenderer : public Renderer {
	public:
		void DrawMesh(Mesh& mesh, Shader& shader) override;
		void SetClearColor() override;
		void Clear() override;
	};

}

