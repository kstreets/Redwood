#pragma once
#include "renderer/Buffer.h"

namespace rwd {

	class GLVertexBuffer : public VertexBuffer {
	public:
		GLVertexBuffer(f32* verts, u32 size);
		~GLVertexBuffer();

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		u32 mId;
	};

	class GLIndexBuffer : public IndexBuffer {
	public:
		GLIndexBuffer(i32* indices, u32 size);
		~GLIndexBuffer();

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		u32 mId;
	};

	class GLVertexArray {
	public:
		GLVertexArray();
		~GLVertexArray();
		void Bind() const;
		void SetVertexBuffer(Ref<GLVertexBuffer> vertexBuffer);
		void SetIndexBuffer(Ref<GLIndexBuffer> indexBuffer);
	private:
		Ref<GLVertexBuffer> mVertexBuffer;
		Ref<GLIndexBuffer> mIndexBuffer;
		u32 mVao;
	};

}

