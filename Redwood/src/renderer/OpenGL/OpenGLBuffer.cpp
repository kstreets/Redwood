#include "pch.h"
#include "glad/glad.h"
#include "OpenGLBuffer.h"

namespace rwd {

	GLVertexBuffer::GLVertexBuffer(f32* verts, u32 size) {
		glCreateBuffers(1, &mId);
		glBindBuffer(GL_ARRAY_BUFFER, mId);
		glBufferData(GL_ARRAY_BUFFER, size, verts, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	GLVertexBuffer::~GLVertexBuffer() {
		glDeleteBuffers(1, &mId);
	}

	void GLVertexBuffer::Bind() const {
		glBindBuffer(GL_ARRAY_BUFFER, mId);
	}

	void GLVertexBuffer::BufferData(const u8* bytes) {

	}

	//-------------------------------------------------------------------------
	//
	// Index Buffer 
	//
	//-------------------------------------------------------------------------

	GLIndexBuffer::GLIndexBuffer(i32* indices, u32 size) {
		glCreateBuffers(1, &mId);
		glBindBuffer(GL_ARRAY_BUFFER, mId);
		glBufferData(GL_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	}

	GLIndexBuffer::~GLIndexBuffer() {
		glDeleteBuffers(1, &mId);
	}

	void GLIndexBuffer::Bind() const {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mId);
	}

	void GLIndexBuffer::BufferData(const u8* bytes) {

	}

	//-------------------------------------------------------------------------
	//
	// Vertex Array 
	//
	//-------------------------------------------------------------------------

	GLVertexArray::GLVertexArray() {
		glGenVertexArrays(1, &mVao);
		glBindVertexArray(mVao);
	}

	GLVertexArray::~GLVertexArray() {
		glDeleteVertexArrays(1, &mVao);
	}

	void GLVertexArray::Bind() const {
		glBindVertexArray(mVao);
	}

	void GLVertexArray::SetVertexBuffer(Ref<GLVertexBuffer> vertexBuffer) {
		glBindVertexArray(mVao);
		vertexBuffer->Bind();
		mVertexBuffer = vertexBuffer;
	}

	void GLVertexArray::SetIndexBuffer(Ref<GLIndexBuffer> indexBuffer) {
		glBindVertexArray(mVao);
		indexBuffer->Bind();
		mIndexBuffer = indexBuffer;
	}

}
