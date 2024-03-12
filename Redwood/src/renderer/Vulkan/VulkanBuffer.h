#pragma once
#include "vulkan/vulkan.hpp"
#include "VulkanContext.h"
#include "renderer/Buffer.h"

namespace rwd {

	class VulkanVertexBuffer : public VertexBuffer {
	public:
		VulkanVertexBuffer() = default;
		VulkanVertexBuffer(void* verts, u32 size, VkDevice device, VmaAllocator allocator);

		void FreeBuffer(VkDevice device, VmaAllocator allocator);
		void FreeStagingBuffer(VkDevice device, VmaAllocator allocator);

		void Bind() const override;
		void BufferData(const u8* bytes) override;

		size_t Size() const;
		VkBuffer Buffer() const;
		VkBuffer StagingBuffer() const;
	private:
		VkBuffer mStagingBuffer;
		VmaAllocation mStagingBufferMemory;

		VkBuffer mBuffer;
		VmaAllocation mBufferMemory;

		size_t mSize;
	};

	class VulkanIndexBuffer : public IndexBuffer {
	public:
		VulkanIndexBuffer() = default;
		VulkanIndexBuffer(void* indices, u32 size, VkDevice device, VmaAllocator allocator);

		void FreeBuffer(VkDevice device, VmaAllocator allocator);
		void FreeStagingBuffer(VkDevice device, VmaAllocator allocator);

		void Bind() const override;
		void BufferData(const u8* bytes) override;

		size_t Size() const;
		VkBuffer Buffer() const;
		VkBuffer StagingBuffer() const;
	private:
		VkBuffer mStagingBuffer;
		VmaAllocation mStagingBufferMemory;

		VkBuffer mBuffer;
		VmaAllocation mBufferMemory;

		size_t mSize;
	};

	class VulkanMesh {
	public:
		VulkanMesh();
		~VulkanMesh();
		void Bind() const;

		void SetVertexBuffer(VulkanVertexBuffer vertexBuffer);
		void SetIndexBuffer(VulkanIndexBuffer indexBuffer);

		size_t VertexBufferSize() const;
		size_t IndexBufferSize() const;

		VkBuffer VertexBuffer() const;
		VkBuffer VertexStagingBuffer() const;

		VkBuffer IndexBuffer() const;
		VkBuffer IndexStagingBuffer() const;
	private:
		VulkanVertexBuffer mVertexBuffer;
		VulkanIndexBuffer mIndexBuffer;
	};

}

