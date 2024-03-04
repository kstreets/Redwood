#pragma once
#include "core/Core.h"
#include "Buffer.h"

namespace rwd {

	class GLVertexArray;

	class Mesh {
	public:
		Mesh(f32* verts, u32 vertSize, i32* indices, u32 indiceSize, i32 indexCount);
	public:
		Ref<GLVertexArray> mGLVertexArray;
		i32 mIndexCount;
	};

}

