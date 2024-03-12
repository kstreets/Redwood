#include "pch.h"
#include "Buffer.h"
#include "OpenGL/OpenGLBuffer.h"
#include "Mesh.h"

namespace rwd {

	Mesh::Mesh(std::vector<f32> verts, std::vector<u32> indices) {
		mVerts = verts;
		mIndices = indices;
	}

	size_t Mesh::VertexBufferSize() const {
		return sizeof(mVerts[0]) * mVerts.size();
	}

	size_t Mesh::IndexBufferSize() const {
		return sizeof(mIndices[0]) * mIndices.size();
	}

}
