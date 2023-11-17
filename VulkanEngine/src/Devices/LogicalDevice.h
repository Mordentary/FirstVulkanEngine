#pragma once

#include "Swapchain.h"  

namespace vkEngine
{
	class PhysicalDevice;
	class Instance;

	class LogicalDevice
	{
	public:
		LogicalDevice(const Shared<PhysicalDevice>& context, const Shared<Instance>& inst, const std::vector<const char*>& deviceExtensions);
		~LogicalDevice();

		LogicalDevice(const LogicalDevice&) = delete;
		LogicalDevice(LogicalDevice&&) noexcept;

		LogicalDevice& operator=(const LogicalDevice&) = delete;
		LogicalDevice& operator=(LogicalDevice&&) = default;

		VkDevice logicalDevice() const { return m_Device; }


	private:
		const Shared<PhysicalDevice>& m_PhysicalDeviceRef;
		const Shared<Instance>& m_InstanceRef;
		VkDevice m_Device{ VK_NULL_HANDLE };
		const std::vector<const char*>& m_DeviceExtensions;

	private:
		void initLogicalDevice();
		void cleanup();
	};
}
