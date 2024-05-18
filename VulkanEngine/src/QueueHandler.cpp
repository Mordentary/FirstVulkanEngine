#include "pch.h"

#include "QueueHandler.h"

namespace vkEngine
{
	QueueHandler::QueueHandler(const Shared<LogicalDevice>& device, const Shared<PhysicalDevice>& physicalDevice)
		: m_Device(device),
		 m_PhysicalDevice(physicalDevice)
	{
		initQueues();
	}

	void QueueHandler::initQueues()
	{
		m_QueueIndices = m_PhysicalDevice->getAvaibleQueueFamilies();
		queryQueues();
	}

	void QueueHandler::submitCommands(VkSubmitInfo submitInfo, VkFence fence)
	{
		ENGINE_ASSERT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence) == VK_SUCCESS, "Queue submition failed");
	}

	void QueueHandler::waitForIdle()
	{
		vkQueueWaitIdle(m_GraphicsQueue);
	}

	void QueueHandler::submitAndWait(VkSubmitInfo submitInfo)
	{
		VkFence fence;
		VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		vkCreateFence(m_Device->logicalDevice(), &fenceCreateInfo, nullptr, &fence);

		vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence);
		vkWaitForFences(m_Device->logicalDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
		vkDestroyFence(m_Device->logicalDevice(), fence, nullptr);
	}

	void QueueHandler::submitAndWaitIdle(VkSubmitInfo submitInfo)
	{
		submitCommands(submitInfo);
		waitForIdle();
	}

	void QueueHandler::queryQueues()
	{
		QueueFamilyIndex graphicsFamily = m_QueueIndices.graphicsFamily.value();
		QueueFamilyIndex presentFamily = m_QueueIndices.presentFamily.value();

		vkGetDeviceQueue(m_Device->logicalDevice(), graphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device->logicalDevice(), presentFamily, 0, &m_PresentQueue);
	}
}