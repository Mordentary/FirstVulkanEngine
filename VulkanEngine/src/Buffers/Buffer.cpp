#include"pch.h"
#include "Buffer.h"
#include "QueueHandler.h"
#include "VulkanContext.h"

namespace vkEngine {
	Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		initBuffer(size, usage, properties);
	}

	Buffer::~Buffer() {
		vkDestroyBuffer(VulkanContext::getDevice(), m_Buffer, nullptr);
		vkFreeMemory(VulkanContext::getDevice(), m_Memory, nullptr);
	}

	void Buffer::copyData(void* data, VkDeviceSize size) const {
		void* mappedMemory;
		vkMapMemory(VulkanContext::getDevice(), m_Memory, 0, size, 0, &mappedMemory);
		memcpy(mappedMemory, data, static_cast<size_t>(size));
		vkUnmapMemory(VulkanContext::getDevice(), m_Memory);
	}

	void Buffer::copyBuffer(VkBuffer srcBuffer, VkDeviceSize size) const {
		VkCommandBuffer commandBuffer = VulkanContext::getCommandHandler()->beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, m_Buffer, 1, &copyRegion);

		VulkanContext::getCommandHandler()->endSingleTimeCommands(commandBuffer);
	}

	Buffer::Buffer(Buffer&& other) noexcept
		: m_Buffer(other.m_Buffer), m_Memory(other.m_Memory) 
	{
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
	}

	Buffer& Buffer::operator=(Buffer&& other) noexcept {
		if (this != &other) {
			vkDestroyBuffer(VulkanContext::getDevice(), m_Buffer, nullptr);
			vkFreeMemory(VulkanContext::getDevice(), m_Memory, nullptr);

			m_Buffer = other.m_Buffer;
			m_Memory = other.m_Memory;

			other.m_Buffer = VK_NULL_HANDLE;
			other.m_Memory = VK_NULL_HANDLE;
		}
		return *this;
	}

	void Buffer::initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateBuffer(VulkanContext::getDevice(), &bufferInfo, nullptr, &m_Buffer) == VK_SUCCESS, "Buffer creation failed");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VulkanContext::getDevice(), m_Buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties);

		ENGINE_ASSERT(vkAllocateMemory(VulkanContext::getDevice(), &allocInfo, nullptr, &m_Memory) == VK_SUCCESS, "Memory allocation failed");

		vkBindBufferMemory(VulkanContext::getDevice(), m_Buffer, m_Memory, 0);
	}

	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices)
		: Buffer(sizeof(vertices[0])* vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		Buffer stagingBufferObj(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBufferObj.copyData((void*)vertices.data(), bufferSize);

		copyBuffer(stagingBufferObj.getBuffer(), bufferSize);
	}

	IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices)
		: Buffer(sizeof(uint32_t)* indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	{
		VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

		Buffer stagingBufferObj(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBufferObj.copyData((void*)indices.data(), bufferSize);

		copyBuffer(stagingBufferObj.getBuffer(), bufferSize);
	}

}