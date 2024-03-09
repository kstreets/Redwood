#pragma once
#include "vulkan/vulkan.hpp"
#include "VulkanContext.h"
#include "renderer/Buffer.h"

namespace rwd {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		friend class VulkanContext;

		VulkanVertexBuffer(void* verts, u32 size);
		~VulkanVertexBuffer();

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		VkBuffer mBuffer;
		VmaAllocation mBufferMemory;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(i32* indices, u32 size);
		~VulkanIndexBuffer();

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		u32 mId;
	};

	class VulkanVertexArray {
	public:
		VulkanVertexArray();
		~VulkanVertexArray();
		void Bind() const;
		void SetVertexBuffer(Ref<VulkanVertexBuffer> vertexBuffer);
		void SetIndexBuffer(Ref<VulkanIndexBuffer> indexBuffer);
	private:
		Ref<VulkanVertexBuffer> mVertexBuffer;
		Ref<VulkanIndexBuffer> mIndexBuffer;
		u32 mVao;
	};

}

