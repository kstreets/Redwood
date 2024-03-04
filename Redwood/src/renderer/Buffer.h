#pragma once
#include "core/Core.h"

namespace rwd {

	class VertexBuffer {
	public:
		virtual ~VertexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void BufferData(const u8* bytes) = 0;
	};

	class IndexBuffer {
	public:
		virtual ~IndexBuffer() = default;

		virtual void Bind() const = 0;
		virtual void BufferData(const u8* bytes) = 0;
	};

}

