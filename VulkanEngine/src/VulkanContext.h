#pragma once

#include <unordered_set>
#include <optional>
#include "ValidationLayersManager.h"
#include "Window/Window.h"

namespace vkEngine
{

	class Engine;
	class Application;
	class QueueHandler;
	using QueueFamilyIndex = uint32_t;

	class VulkanContext
	{
	public:
		VulkanContext(const Engine* engine, const std::vector<const char*>& deviceExtensions);
		~VulkanContext();

		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; };
		VkDevice getDevice() const { return m_Device; };
		VkPhysicalDeviceFeatures getDeviceEnabledFeatures() const { return m_EnabledFeatures; }
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice) const;
		VkBool32 isQueueSupportPresentation(VkPhysicalDevice device, QueueFamilyIndex index) const;
		const Shared<QueueHandler> getQueueHandler() const { return m_QueueHandler; }
		const Shared<Swapchain> getSwapchain() { return m_Swapchain; }




		//TODO: Find the better place for functions
		VkFormat findDepthFormat();
		bool hasStencilComponent(VkFormat format);
		QueueFamilyIndices getAvaibleQueueFamilies() const;
	private:
		void cleanup();
		void initialize();
	private:
		const Engine* m_Engine = nullptr;
		Shared<Swapchain> m_Swapchain = nullptr;
		Shared<QueueHandler> m_QueueHandler = nullptr;

		VkPhysicalDevice m_PhysicalDevice{ VK_NULL_HANDLE };
		VkPhysicalDeviceFeatures m_EnabledFeatures{};
		VkDevice m_Device{ VK_NULL_HANDLE };

		std::vector<const char*> m_DeviceExtensions;
	private:
		// Helper functions
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		void pickPhysicalDevice();
		void initLogicalDevice();
		inline void initSwapchain();
		inline void initQueueHandler();
		VkPhysicalDeviceFeatures getSupportedFeatures() const;
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool isDeviceSuitable(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkQueueFlagBits flags) const;
	};
}
