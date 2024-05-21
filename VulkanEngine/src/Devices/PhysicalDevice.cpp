#include "pch.h"
#include "PhysicalDevice.h"
#include "QueueHandler.h"
#include "vulkan/vulkan.h"

namespace vkEngine
{
	PhysicalDevice::PhysicalDevice(const Shared<Instance>& inst, const Shared<Window>& win, const std::vector<const char*>& deviceExt)
		:
		m_Instance(inst),
		m_Window(win),
		m_DeviceExtensions(deviceExt)
	{
		initialize();
	}

	bool PhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::unordered_set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails PhysicalDevice::querySwapChainSupport() const
	{
		VkSurfaceKHR surface = m_Window->getSurface();

		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentMode;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentMode, nullptr);
		if (presentMode != 0)
		{
			details.presentModes.resize(presentMode);
			vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, surface, &presentMode, details.presentModes.data());
		}

		return details;
	}

	uint32_t PhysicalDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties = getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
			{
				return i;
			}
		}

		ENGINE_ASSERT(false, "There is no suitable type of memory for buffer allocation");
		return 0;
	}

	VkFormat PhysicalDevice::findDepthFormat()
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	bool PhysicalDevice::hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkFormat PhysicalDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props = getFormatProperties(format);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}

		}
		ENGINE_ASSERT(false, "No suitable formats was found")
			return VK_FORMAT_UNDEFINED;
	}
	VkFormatProperties PhysicalDevice::getFormatProperties(VkFormat format) const
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
		return props;
	}

	SwapChainSupportDetails PhysicalDevice::querySwapChainSupport(VkPhysicalDevice physicalDevice) const
	{
		VkSurfaceKHR surface = m_Window->getSurface();

		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
		}

		uint32_t presentMode;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentMode, nullptr);
		if (presentMode != 0)
		{
			details.presentModes.resize(presentMode);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentMode, details.presentModes.data());
		}

		return details;
	}

	bool PhysicalDevice::isDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = findQueueFamilies(device, VK_QUEUE_GRAPHICS_BIT);
		bool extensSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;

		if (extensSupported)
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return
			deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&
			indices.isComplete()
			&&
			extensSupported
			&&
			swapChainAdequate;
	}

	VkBool32 PhysicalDevice::isQueueSupportPresentation(VkPhysicalDevice device, QueueFamilyIndex index) const
	{
		VkBool32 presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_Window->getSurface(), &presentSupport);
		return presentSupport;
	}

	QueueFamilyIndices PhysicalDevice::findQueueFamilies(VkPhysicalDevice device, VkQueueFlagBits flags) const
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT & flags)
			{
				indices.graphicsFamily = i;
			}

			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT & flags)
			{
				indices.computeFamily = i;
			}

			if (isQueueSupportPresentation(device, i))
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}

			i++;
		}
		return indices;
	}


	QueueFamilyIndices PhysicalDevice::getAvaibleQueueFamilies() const
	{
		return findQueueFamilies(m_PhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
	}


	void PhysicalDevice::initialize()
	{
		VkInstance getInstance = m_Instance->instance();
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(getInstance, &deviceCount, nullptr);
		ENGINE_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(getInstance, &deviceCount, devices.data());

		deviceCount = 0;
		VkPhysicalDeviceProperties pickedDeviceProp;
		for (auto& device : devices)
		{
			vkGetPhysicalDeviceProperties(device, &pickedDeviceProp);
			if (VK_NULL_HANDLE == m_PhysicalDevice && isDeviceSuitable(device))
			{
				m_PhysicalDevice = device;

				vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceInfo.properties);
				vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_DeviceInfo.features);
				vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_DeviceInfo.memoryProperties);
				uint32_t queueFamilyCount = 0;
				vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
				m_DeviceInfo.queueFamiliesProperties.resize(queueFamilyCount);
				vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_DeviceInfo.queueFamiliesProperties.data());

				ENGINE_INFO("--> Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);
			}
			else
				ENGINE_INFO("Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);

			deviceCount++;
		}

		ENGINE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find suitable GPU");
	}
}