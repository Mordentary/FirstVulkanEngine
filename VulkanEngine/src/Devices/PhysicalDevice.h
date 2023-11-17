#pragma once

#include"Instance.h"
#include"../Window/Window.h"

namespace vkEngine
{

	struct PhysicalDeviceInfo
	{
		VkPhysicalDeviceProperties properties{};
		VkPhysicalDeviceFeatures features{};
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		std::vector<VkQueueFamilyProperties> queueFamiliesProperties{};

	};
	class PhysicalDevice
	{
	public:
		PhysicalDevice(const Shared<Instance>& inst, const Shared<Window>& win, const std::vector<const char*>& deviceExtensions);

		VkPhysicalDevice physicalDevice() const { return m_PhysicalDevice; }
		VkPhysicalDeviceProperties getProperties() const { return m_DeviceInfo.properties; }
		VkPhysicalDeviceFeatures getFeatures() const { return m_DeviceInfo.features; }
		VkPhysicalDeviceMemoryProperties getMemoryProperties() const { return m_DeviceInfo.memoryProperties; }
		VkQueueFamilyProperties getQueueFamilyProperties(uint32_t index) const { return m_DeviceInfo.queueFamiliesProperties[index]; }
		QueueFamilyIndices getAvaibleQueueFamilies() const;
		SwapChainSupportDetails querySwapChainSupport() const;

		const PhysicalDeviceInfo& getDeviceInfo() const { return m_DeviceInfo; }

		VkFormatProperties getFormatProperties(VkFormat format) const;
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		PhysicalDeviceInfo m_DeviceInfo;
		const Shared<Instance>& m_InstanceRef;
		const Shared<Window>& m_WindowRef;
		const std::vector<const char*>& m_DeviceExtensions{};

	private:
		void initialize();
		bool isDeviceSuitable(VkPhysicalDevice device);
		VkBool32 isQueueSupportPresentation(VkPhysicalDevice device, QueueFamilyIndex index) const;
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkQueueFlagBits flags) const;
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice PhysicalDevice) const;
	
	};
}
