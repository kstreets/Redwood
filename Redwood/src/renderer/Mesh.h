#pragma once
#include "pch.h"
#include "core/Core.h"

namespace rwd {

	class Mesh {
	public:
		Mesh(std::vector<f32> verts, std::vector<u32> indices);

		size_t VertexBufferSize() const;
		size_t IndexBufferSize() const;

	public:
		std::vector<f32> mVerts;
		std::vector<u32> mIndices;
	};

}

