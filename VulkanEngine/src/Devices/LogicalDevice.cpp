#include "pch.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include"QueueHandler.h"

namespace vkEngine
{
	LogicalDevice::LogicalDevice(const Shared<PhysicalDevice>& device, const Shared<Instance>& inst, const std::vector<const char*>& deviceExtensions)
		: m_PhysicalDeviceRef(device),
		m_DeviceExtensions(deviceExtensions),
		m_InstanceRef(inst)
	{
		initLogicalDevice();
	}

	LogicalDevice::~LogicalDevice()
	{
		cleanup();
	}

	void LogicalDevice::initLogicalDevice()
	{
		QueueFamilyIndices indices = m_PhysicalDeviceRef->getAvaibleQueueFamilies();

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

		VkPhysicalDeviceFeatures deviceFeatures = m_PhysicalDeviceRef->getFeatures();

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceInfo.pEnabledFeatures = &deviceFeatures;

		//Specify validation layers for older version of vulkan where is still the case (Previosly vulkan differ these two settings)
		std::vector<const char*> validationLayers{};
		if (m_InstanceRef->isValidationLayersEnabled())
		{
			validationLayers = m_InstanceRef->getActivatedValidationLayers();
			deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			deviceInfo.enabledLayerCount = 0;
		}

		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

		ENGINE_ASSERT(vkCreateDevice(m_PhysicalDeviceRef->physicalDevice(), &deviceInfo, nullptr, &m_Device) == VK_SUCCESS, "Device creation failed");
	}

	void LogicalDevice::cleanup()
	{
		ENGINE_ASSERT(m_Device != VK_NULL_HANDLE, "Logical Device is null");
		vkDestroyDevice(m_Device, nullptr);
		m_Device = VK_NULL_HANDLE;
	}


}



