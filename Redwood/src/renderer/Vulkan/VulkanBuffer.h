#pragma once
#include "vulkan/vulkan.hpp"
#include "VulkanContext.h"
#include "renderer/Buffer.h"

namespace rwd {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer(void* verts, u32 size);
		~VulkanVertexBuffer();

		operator VkBuffer() const { return mBuffer; };

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		VkBuffer mBuffer;
		VmaAllocation mBufferMemory;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer(void* indices, u32 size);
		~VulkanIndexBuffer();

		operator VkBuffer() const { return mBuffer; };

		void Bind() const override;
		void BufferData(const u8* bytes) override;
	private:
		VkBuffer mBuffer;
		VmaAllocation mBufferMemory;
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

