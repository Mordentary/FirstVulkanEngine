#pragma once

#include "VulkanContext.h"

namespace vkEngine
{
	using QueueFamilyIndex = uint32_t;
	struct QueueFamilyIndices
	{
	public:
		uint32_t uniqueQueueFamilyCount()
		{
			std::unordered_set<QueueFamilyIndex> uniqueFamilies;
			if (graphicsFamily.has_value())
				uniqueFamilies.insert(graphicsFamily.value());
			if (presentFamily.has_value())
				uniqueFamilies.insert(presentFamily.value());
			if (computeFamily.has_value())
				uniqueFamilies.insert(computeFamily.value());

			return static_cast<uint32_t>(uniqueFamilies.size());
		}

		std::unordered_set<QueueFamilyIndex> uniqueQueueFamilies()
		{
			std::unordered_set<QueueFamilyIndex> uniqueFamilies;
			if (graphicsFamily.has_value())
				uniqueFamilies.insert(graphicsFamily.value());
			if (presentFamily.has_value())
				uniqueFamilies.insert(presentFamily.value());
			if (computeFamily.has_value())
				uniqueFamilies.insert(computeFamily.value());
			return uniqueFamilies;
		}
		//TODO: compute family integration
		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	public:
		std::optional<QueueFamilyIndex> graphicsFamily;
		std::optional<QueueFamilyIndex> presentFamily;
		std::optional<QueueFamilyIndex> computeFamily;
	};

	class QueueHandler
	{
	public:
		QueueHandler(const VulkanContext& context);

		void submitCommands(VkSubmitInfo submitInfo, VkFence fence = VK_NULL_HANDLE);
		void waitForIdle();
		void submitAndWait(VkSubmitInfo submitInfo);
		void submitAndWaitIdle(VkSubmitInfo submitInfo);

		bool isGraphicsQueueSupported() const { return m_QueueIndices.graphicsFamily.has_value(); }
		bool isPresentQueueSupported() const { return m_QueueIndices.presentFamily.has_value(); }
		bool supportsAllIndices() const { return m_QueueIndices.graphicsFamily.has_value() && m_QueueIndices.presentFamily.has_value() && m_QueueIndices.computeFamily.has_value(); }
		QueueFamilyIndices getQueueFamilyIndices() const { return m_QueueIndices; };
		VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue getPresentQueue() const { return m_PresentQueue; }
	private:
		void initQueues();
		void queryQueues();
	private:
		const VulkanContext& m_Context;
		VkDevice m_Device;
		QueueFamilyIndices m_QueueIndices;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
	};
}
