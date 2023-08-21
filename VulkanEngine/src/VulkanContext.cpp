#include "VulkanContext.h"
#include "Logger/Logger.h"
#include "Application.h"
#include "QueueHandler.h"

namespace vkEngine
{
	VulkanContext::VulkanContext(const Engine* engine, const std::vector<const char*>& deviceExtensions)
		: m_DeviceExtensions(deviceExtensions), m_Engine(engine)
	{
		initialize();
	}

	VulkanContext::~VulkanContext()
	{
		cleanup();
	}

	void VulkanContext::initialize()
	{
		pickPhysicalDevice();
		m_EnabledFeatures = getSupportedFeatures();
		initLogicalDevice();
		initQueueHandler();
		initSwapchain();
	}

	void VulkanContext::initSwapchain()
	{
		m_Swapchain = CreateScoped<Swapchain>
			(
				m_Engine->getApp()->getWindow(),
				m_Engine->getApp()->getSurface(),
				*this,
				m_Engine->s_MaxFramesInFlight
			);
	}


	void VulkanContext::initQueueHandler()
	{
		m_QueueHandler = CreateScoped<QueueHandler>(*this);
	}

	SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice physicalDevice) const
	{
		VkSurfaceKHR surface = m_Engine->getApp()->getSurface();


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

	VkBool32 VulkanContext::isQueueSupportPresentation(VkPhysicalDevice device, QueueFamilyIndex index) const
	{
		VkBool32 presentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_Engine->getApp()->getSurface(), &presentSupport);
		return presentSupport;
	}

	VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
			else
				ENGINE_ASSERT(false, "No suitable formats was found")

		}
	}

	VkFormat VulkanContext::findDepthFormat()
	{
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	bool VulkanContext::hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}




	void VulkanContext::cleanup()
	{
		m_Swapchain->cleanupSwapchain();
		vkDestroyDevice(m_Device, nullptr);
	}

	void VulkanContext::pickPhysicalDevice()
	{
		VkInstance instance = m_Engine->getApp()->getInstance();
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		ENGINE_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		deviceCount = 0;
		VkPhysicalDeviceProperties pickedDeviceProp;
		for (auto& device : devices)
		{
			vkGetPhysicalDeviceProperties(device, &pickedDeviceProp);
			if (VK_NULL_HANDLE == m_PhysicalDevice && isDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				ENGINE_INFO("--> Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);
			}
			else
				ENGINE_INFO("Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);

			deviceCount++;
		}

		ENGINE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "Failed to find suitable GPU");
	}
	void VulkanContext::initLogicalDevice()
	{
		QueueFamilyIndices indices = getAvaibleQueueFamilies();

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		queueCreateInfos.reserve(indices.uniqueQueueFamilyCount());

		std::unordered_set<QueueFamilyIndex> uniqueIndices = indices.uniqueQueueFamilies();
		float queuePriority = 1.0f;
		for (QueueFamilyIndex family : uniqueIndices)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = getSupportedFeatures();


		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceInfo.pEnabledFeatures = &m_EnabledFeatures;

		//Specify validation layers for older version of vulkan where is still the case (Previosly vulkan differ these two settings)
		std::vector<const char*> validationLayers{};
		if (m_Engine->getApp()->isValidationLayersEnabled())
		{
			validationLayers = m_Engine->getApp()->getActivatedValidationLayers();
			deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			deviceInfo.enabledLayerCount = 0;
		}

		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		ENGINE_ASSERT(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_Device) == VK_SUCCESS, "Device creation failed");
	}
	bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device)
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
			indices.isComplete()
			&&
			extensSupported
			&&
			swapChainAdequate;
	}

	QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device, VkQueueFlagBits flags) const
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

	QueueFamilyIndices VulkanContext::getAvaibleQueueFamilies() const
	{
		return findQueueFamilies(m_PhysicalDevice, VK_QUEUE_GRAPHICS_BIT);
	}
	VkPhysicalDeviceFeatures VulkanContext::getSupportedFeatures() const
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
		vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

		if (deviceProperties.limits.maxSamplerAnisotropy > 1)
			deviceFeatures.samplerAnisotropy = VK_TRUE;

		return deviceFeatures;
	}
	bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device)
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

}