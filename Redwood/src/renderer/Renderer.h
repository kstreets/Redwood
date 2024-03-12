#pragma once

namespace rwd {

	class Mesh;
	class Shader;

	class Renderer {
	public:
		virtual void DrawMesh(Mesh& mesh, Shader& shader) = 0;
		virtual void SetClearColor() = 0;
		virtual void Clear() = 0;
	};

}
