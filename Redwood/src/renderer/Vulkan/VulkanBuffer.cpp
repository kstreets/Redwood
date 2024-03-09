#include "pch.h"
#include "core/Log.h"
#include "VulkanBuffer.h"

namespace rwd {

	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, 
		VkBuffer& buffer, VmaAllocation& bufferMemory) 
	{
		VkBufferCreateInfo bufferInfo { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkDevice device = VulkanContext::VulkanDevice();
		VmaAllocator allocator = VulkanContext::CustomAllocator();

		VmaAllocationCreateInfo allocInfo { };
		allocInfo.usage = memoryUsage;

		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &bufferMemory, nullptr);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer");
	}
	
	VulkanVertexBuffer::VulkanVertexBuffer(void* verts, u32 size) {
		VkDevice device = VulkanContext::VulkanDevice();
		VmaAllocator allocator = VulkanContext::CustomAllocator();

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferMemory;

		VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		CreateBuffer(size, stagingUsage, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer, stagingBufferMemory);

		void* data;
		vmaMapMemory(allocator, stagingBufferMemory, &data);
		memcpy(data, verts, size);
		vmaUnmapMemory(allocator, stagingBufferMemory);

		VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		CreateBuffer(size, vertexUsage, VMA_MEMORY_USAGE_GPU_ONLY, mBuffer, mBufferMemory);

		VulkanContext::CopyBuffer(stagingBuffer, mBuffer, size);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vmaFreeMemory(allocator, stagingBufferMemory);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer() {
		VkDevice device = VulkanContext::VulkanDevice();
		VmaAllocator allocator = VulkanContext::CustomAllocator();

		vmaFreeMemory(allocator, mBufferMemory);
		vkDestroyBuffer(device, mBuffer, nullptr);
	}

	void VulkanVertexBuffer::Bind() const {

	}

	void VulkanVertexBuffer::BufferData(const u8* bytes) {

	}

	//-------------------------------------------------------------------------
	//
	// Index Buffer 
	//
	//-------------------------------------------------------------------------

	VulkanIndexBuffer::VulkanIndexBuffer(i32* indices, u32 size) {
	}

	VulkanIndexBuffer::~VulkanIndexBuffer() {
	}

	void VulkanIndexBuffer::Bind() const {
	}

	void VulkanIndexBuffer::BufferData(const u8* bytes) {

	}

	//-------------------------------------------------------------------------
	//
	// Vertex Array 
	//
	//-------------------------------------------------------------------------

	VulkanVertexArray::VulkanVertexArray() {
	}

	VulkanVertexArray::~VulkanVertexArray() {
	}

	void VulkanVertexArray::Bind() const {
	}

	void VulkanVertexArray::SetVertexBuffer(Ref<VulkanVertexBuffer> vertexBuffer) {
	}

	void VulkanVertexArray::SetIndexBuffer(Ref<VulkanIndexBuffer> indexBuffer) {
	}

}
