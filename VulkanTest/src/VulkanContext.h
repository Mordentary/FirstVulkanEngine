#pragma once

#include <unordered_set>
#include <optional>
#include "ValidationLayersManager.h"
#include "Window/Window.h"

namespace vkEngine
{

	class Engine;
	class Application;
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

			return static_cast<uint32_t>(uniqueFamilies.size());
		}

		std::unordered_set<QueueFamilyIndex> uniqueQueueFamilies()
		{
			std::unordered_set<QueueFamilyIndex> uniqueFamilies;
			if (graphicsFamily.has_value())
				uniqueFamilies.insert(graphicsFamily.value());
			if (presentFamily.has_value())
				uniqueFamilies.insert(presentFamily.value());

			return uniqueFamilies;
		}

		bool isComplete()
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	public:
		std::optional<QueueFamilyIndex> graphicsFamily;
		std::optional<QueueFamilyIndex> presentFamily;
	};

	class VulkanContext
	{
	public:
		VulkanContext(const Engine* engine, const std::vector<const char*>& deviceExtensions);
		~VulkanContext();

		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; };
		VkDevice getDevice() const { return m_Device; };
		VkQueue getGraphicsQueue() const { return m_GraphicsQueue; };
		VkQueue getPresentQueue() const { return m_PresentQueue; };
		QueueFamilyIndices getQueueIndices() const { return m_QueueIndices; }
		VkPhysicalDeviceFeatures getDeviceEnabledFeatures() const { return m_EnabledFeatures; }


	private:
		void queryQueues();
		void cleanup();
		void initialize();
	private:
		const Engine* m_Engine = nullptr;
		VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
		VkPhysicalDeviceFeatures m_EnabledFeatures{};
		VkDevice m_Device{ VK_NULL_HANDLE };

		QueueFamilyIndices m_QueueIndices;
		//TODO: Class for queues
		VkQueue m_GraphicsQueue{ VK_NULL_HANDLE }, m_PresentQueue{ VK_NULL_HANDLE };

		std::vector<const char*> m_DeviceExtensions;
	private:
		// Helper functions for Vulkan initialization
		void pickPhysicalDevice();
		void initLogicalDevice();

		VkPhysicalDeviceFeatures getSupportedFeatures();
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	};
}
