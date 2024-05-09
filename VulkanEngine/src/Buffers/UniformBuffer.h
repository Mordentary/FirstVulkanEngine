#pragma once
#include "Buffer.h"	


namespace vkEngine
{
	class UniformBuffer : public Buffer
	{
	public:
		UniformBuffer(VkDeviceSize size);
		~UniformBuffer();

		UniformBuffer(const UniformBuffer&) = delete;
		UniformBuffer& operator=(const UniformBuffer&) = delete;
		UniformBuffer(UniformBuffer&& other) noexcept;
		UniformBuffer& operator=(UniformBuffer&& other) noexcept;

		void mapMemory();
		void unmapMemory();
		void* getMappedMemory();
	private:
		void* m_MappedMemoryPtr;
	};
}
