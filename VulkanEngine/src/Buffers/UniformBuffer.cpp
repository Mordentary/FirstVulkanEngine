#include "pch.h"
#include "UniformBuffer.h"


namespace vkEngine
{
	UniformBuffer::UniformBuffer(VkDeviceSize size)
		: Buffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		m_MappedMemoryPtr(nullptr)
	{
	}

	UniformBuffer::~UniformBuffer()
	{
		if (m_MappedMemoryPtr) {
			vkUnmapMemory(VulkanContext::getDevice(), m_Memory);
			m_MappedMemoryPtr = nullptr;
		}
	}

	UniformBuffer::UniformBuffer(UniformBuffer&& other) noexcept
		: Buffer(std::move(other)), m_MappedMemoryPtr(other.m_MappedMemoryPtr)
	{
		other.m_MappedMemoryPtr = nullptr;
	}

	UniformBuffer& UniformBuffer::operator=(UniformBuffer&& other) noexcept
	{
		if (this != &other) {
			Buffer::operator=(std::move(other));
			m_MappedMemoryPtr = other.m_MappedMemoryPtr;
			other.m_MappedMemoryPtr = nullptr;
		}
		return *this;
	}

	void UniformBuffer::mapMemory()
	{
		vkMapMemory(VulkanContext::getDevice(), m_Memory, 0, VK_WHOLE_SIZE, 0, &m_MappedMemoryPtr);
	}

	void UniformBuffer::unmapMemory()
	{
		vkUnmapMemory(VulkanContext::getDevice(), m_Memory);
	}

	void* UniformBuffer::getMappedMemory()
	{
		ENGINE_ASSERT(m_MappedMemoryPtr, "Mapped memory - nullptr: Uniform buffer")
			return m_MappedMemoryPtr;
	}

}
