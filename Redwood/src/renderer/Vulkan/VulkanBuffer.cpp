#include "pch.h"
#include "core/Log.h"
#include "VulkanContext.h"
#include "VulkanBuffer.h"

namespace rwd {

	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, 
		VkBuffer& buffer, VkDeviceMemory& bufferMemory) 
	{
		VkBufferCreateInfo bufferInfo { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkDevice device = VulkanContext::VulkanDevice();

		VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanContext::FindMemoryType(memRequirements.memoryTypeBits, props);

		result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
		RWD_ASSERT(result == VK_SUCCESS, "Failed to allocate memory for Vulkan vertex buffer");

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* verts, u32 size) {
		VkDevice device = VulkanContext::VulkanDevice();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags stagingProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		CreateBuffer(size, stagingUsage, stagingProps, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
		memcpy(data, verts, size);
		vkUnmapMemory(device, stagingBufferMemory);

		VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		VkMemoryPropertyFlags vertexProps = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
		CreateBuffer(size, vertexUsage, vertexProps, mBuffer, mBufferMemory);

		VulkanContext::CopyBuffer(stagingBuffer, mBuffer, size);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer() {
		VkDevice device = VulkanContext::VulkanDevice();
		vkFreeMemory(device, mBufferMemory, nullptr);
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
