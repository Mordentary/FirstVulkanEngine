#include "pch.h"
#include "CommandBufferHandler.h"
#include "VulkanContext.h"
#include "Core.h"
#include "QueueHandler.h"

namespace vkEngine {
	CommandBufferHandler::CommandBufferHandler(const VkDevice device, uint32_t queueFamilyIndex)
		: m_Device(device)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		ENGINE_ASSERT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) == VK_SUCCESS,
			"Failed to create command pool!");
	}

	CommandBufferHandler::~CommandBufferHandler()
	{
		freeCommandBuffers();
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
	}

	VkCommandBuffer CommandBufferHandler::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		ENGINE_ASSERT(vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer) == VK_SUCCESS,
			"Failed to allocate single-time command buffer!");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		ENGINE_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS,
			"Failed to begin recording single-time command buffer!");

		return commandBuffer;
	}

	void CommandBufferHandler::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
		ENGINE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Failed to end recording single-time command buffer!");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkDevice device = VulkanContext::getDevice();

		VkFence fence;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = 0;
		ENGINE_ASSERT(vkCreateFence(device, &fenceInfo, nullptr, &fence) == VK_SUCCESS, "Failed to create fence for single-time command buffer!");

		ENGINE_ASSERT(vkQueueSubmit(VulkanContext::getQueueHandler()->getGraphicsQueue(), 1, &submitInfo, fence) == VK_SUCCESS,
			"Failed to submit single-time command buffer!");

		ENGINE_ASSERT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) == VK_SUCCESS,
			"Failed to wait for single-time command buffer fence!");

		vkDestroyFence(device, fence, nullptr);
		vkFreeCommandBuffers(device, m_CommandPool, 1, &commandBuffer);
	}

	void CommandBufferHandler::allocateCommandBuffers(uint32_t count) {
		freeCommandBuffers();

		m_CommandBuffers.resize(count);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = count;

		ENGINE_ASSERT(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS,
			"Failed to allocate command buffers!");
	}

	void CommandBufferHandler::freeCommandBuffers() {
		if (!m_CommandBuffers.empty())
		{
			vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
			m_CommandBuffers.clear();
		}
	}

	VkCommandBuffer CommandBufferHandler::getCommandBuffer(uint32_t index) const
	{
		if (index >= m_CommandBuffers.size()) {
			throw std::runtime_error("Command buffer index out of range!");
		}
		return m_CommandBuffers[index];
	}

	VkCommandBuffer CommandBufferHandler::beginCommandBuffer(uint32_t index) {
		ENGINE_ASSERT(index < m_CommandBuffers.size(), "Command buffer index out of range!");

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		ENGINE_ASSERT(vkBeginCommandBuffer(m_CommandBuffers[index], &beginInfo) == VK_SUCCESS,
			"Failed to begin recording command buffer!");

		return m_CommandBuffers[index];
	}

	void CommandBufferHandler::endCommandBuffer(uint32_t index) {
		ENGINE_ASSERT(index < m_CommandBuffers.size(), "Command buffer index out of range!");

		ENGINE_ASSERT(vkEndCommandBuffer(m_CommandBuffers[index]) == VK_SUCCESS,
			"Failed to end recording command buffer!");
	}

	void CommandBufferHandler::submitCommandBuffer(uint32_t index, VkQueue queue, VkFence fence)
	{
		ENGINE_ASSERT(index < m_CommandBuffers.size(), "Command buffer index out of range!");



			VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[index];

		ENGINE_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence) == VK_SUCCESS,
			"Failed to submit command buffer!");
	}
}