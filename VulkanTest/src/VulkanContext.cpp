#include "VulkanContext.h"
#include <Logger/Logger.h>
#include "Application.h"

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
		queryQueues();
	}

	void VulkanContext::queryQueues()
	{
		m_QueueIndices = findQueueFamilies(m_PhysicalDevice);

		QueueFamilyIndex graphicsFamily = m_QueueIndices.graphicsFamily.value();
		QueueFamilyIndex presentFamily = m_QueueIndices.presentFamily.value();

		vkGetDeviceQueue(m_Device, graphicsFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, presentFamily, 0, &m_PresentQueue);
	}

	void VulkanContext::cleanup()
	{
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
		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice);

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

		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;

		if (extensSupported)
		{
			SwapChainSupportDetails swapChainSupport = m_Engine->getApp()->querySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return
			indices.isComplete()
			&&
			extensSupported
			&&
			swapChainAdequate;
	}
	VkPhysicalDeviceFeatures VulkanContext::getSupportedFeatures()
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

	QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Engine->getApp()->getSurface(), &presentSupport);
			if (presentSupport)
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
}