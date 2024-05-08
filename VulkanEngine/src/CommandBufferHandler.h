#pragma once

#include <vector>
#include "Logger/Logger.h"

namespace vkEngine {
	class CommandBufferHandler
	{
	public:
		CommandBufferHandler(VkDevice device, uint32_t queueFamilyIndex);
		~CommandBufferHandler();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		void allocateCommandBuffers(uint32_t count);
		void freeCommandBuffers();

		VkCommandBuffer beginCommandBuffer(uint32_t index);
		void endCommandBuffer(uint32_t index);
		void submitCommandBuffer(uint32_t index, VkQueue queue, VkFence fence = VK_NULL_HANDLE);

		uint32_t getCommandBufferCount() const { return static_cast<uint32_t>(m_CommandBuffers.size()); }
		VkCommandBuffer getCommandBuffer(uint32_t index) const;

	private:
		const VkDevice m_Device = VK_NULL_HANDLE;;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers{};
	};
}
