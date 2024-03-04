#include "pch.h"
#include "Buffer.h"
#include "OpenGL/OpenGLBuffer.h"
#include "Mesh.h"

namespace rwd {

	Mesh::Mesh(f32* verts, u32 vertSize, i32* indices, u32 indiceSize, i32 indexCount) {
		mGLVertexArray = MakeRef<GLVertexArray>();

		Ref<GLVertexBuffer> vertexBuffer = MakeRef<GLVertexBuffer>(verts, vertSize);
		Ref<GLIndexBuffer> indexBuffer = MakeRef<GLIndexBuffer>(indices, indiceSize);

		mGLVertexArray->SetVertexBuffer(vertexBuffer);
		mGLVertexArray->SetIndexBuffer(indexBuffer);

		mIndexCount = indexCount;
	}

}
