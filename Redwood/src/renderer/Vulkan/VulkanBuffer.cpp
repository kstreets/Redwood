#include "pch.h"
#include "core/Log.h"
#include "VulkanBuffer.h"

namespace rwd {

	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage, 
		VkBuffer& buffer, VmaAllocation& bufferMemory, VkDevice device, VmaAllocator allocator) 
	{
		VkBufferCreateInfo bufferInfo { };
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo { };
		allocInfo.usage = memoryUsage;

		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &bufferMemory, nullptr);

		RWD_ASSERT(result == VK_SUCCESS, "Failed to create Vulkan buffer");
	}

	VulkanVertexBuffer::VulkanVertexBuffer(void* verts, u32 size, VkDevice device, VmaAllocator allocator) {
		mSize = size;

		VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		CreateBuffer(size, stagingUsage, VMA_MEMORY_USAGE_CPU_TO_GPU, mStagingBuffer, mStagingBufferMemory, device, allocator);

		void* data;
		vmaMapMemory(allocator, mStagingBufferMemory, &data);
		memcpy(data, verts, size);
		vmaUnmapMemory(allocator, mStagingBufferMemory);

		VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		CreateBuffer(size, vertexUsage, VMA_MEMORY_USAGE_GPU_ONLY, mBuffer, mBufferMemory, device, allocator);
	}

	void VulkanVertexBuffer::FreeBuffer(VkDevice device, VmaAllocator allocator) {
		vmaFreeMemory(allocator, mBufferMemory);
		vkDestroyBuffer(device, mBuffer, nullptr);
	}

	void VulkanVertexBuffer::FreeStagingBuffer(VkDevice device, VmaAllocator allocator) {
		vkDestroyBuffer(device, mStagingBuffer, nullptr);
		vmaFreeMemory(allocator, mStagingBufferMemory);
	}

	void VulkanVertexBuffer::Bind() const {

	}

	void VulkanVertexBuffer::BufferData(const u8* bytes) {

	}

	size_t VulkanVertexBuffer::Size() const {
		return mSize;
	}

	VkBuffer VulkanVertexBuffer::Buffer() const {
		return mBuffer;
	}

	VkBuffer VulkanVertexBuffer::StagingBuffer() const {
		return mStagingBuffer;
	}

	//-------------------------------------------------------------------------
	//
	// Index Buffer 
	//
	//-------------------------------------------------------------------------

	VulkanIndexBuffer::VulkanIndexBuffer(void* indices, u32 size, VkDevice device, VmaAllocator allocator) {
		mSize = size;

		VkBufferUsageFlags stagingUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		CreateBuffer(size, stagingUsage, VMA_MEMORY_USAGE_CPU_TO_GPU, mStagingBuffer, mStagingBufferMemory, device, allocator);

		void* data;
		vmaMapMemory(allocator, mStagingBufferMemory, &data);
		memcpy(data, indices, size);
		vmaUnmapMemory(allocator, mStagingBufferMemory);

		VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		CreateBuffer(size, vertexUsage, VMA_MEMORY_USAGE_GPU_ONLY, mBuffer, mBufferMemory, device, allocator);
	}

	void VulkanIndexBuffer::FreeBuffer(VkDevice device, VmaAllocator allocator) {
		vmaFreeMemory(allocator, mBufferMemory);
		vkDestroyBuffer(device, mBuffer, nullptr);
	}

	void VulkanIndexBuffer::FreeStagingBuffer(VkDevice device, VmaAllocator allocator) {
		vkDestroyBuffer(device, mStagingBuffer, nullptr);
		vmaFreeMemory(allocator, mStagingBufferMemory);
	}

	void VulkanIndexBuffer::Bind() const {

	}

	void VulkanIndexBuffer::BufferData(const u8* bytes) {

	}

	size_t VulkanIndexBuffer::Size() const {
		return mSize;
	}

	VkBuffer VulkanIndexBuffer::Buffer() const {
		return mBuffer;
	}

	VkBuffer VulkanIndexBuffer::StagingBuffer() const {
		return mStagingBuffer;
	}

	//-------------------------------------------------------------------------
	//
	// Vertex Array 
	//
	//-------------------------------------------------------------------------

	VulkanMesh::VulkanMesh() {

	}

	VulkanMesh::~VulkanMesh() {

	}

	void VulkanMesh::Bind() const {
	}

	void VulkanMesh::SetVertexBuffer(VulkanVertexBuffer vertexBuffer) {
		mVertexBuffer = vertexBuffer;
	}

	void VulkanMesh::SetIndexBuffer(VulkanIndexBuffer indexBuffer) {
		mIndexBuffer = indexBuffer;
	}

	size_t VulkanMesh::VertexBufferSize() const {
		return mVertexBuffer.Size();
	}

	size_t VulkanMesh::IndexBufferSize() const {
		return mIndexBuffer.Size();
	}

	VkBuffer VulkanMesh::VertexBuffer() const {
		return mVertexBuffer.Buffer();
	}

	VkBuffer VulkanMesh::VertexStagingBuffer() const {
		return mVertexBuffer.StagingBuffer();
	}

	VkBuffer VulkanMesh::IndexBuffer() const {
		return mIndexBuffer.Buffer();
	}

	VkBuffer VulkanMesh::IndexStagingBuffer() const {
		return mIndexBuffer.StagingBuffer();
	}

}
