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
//		LogicalDevice(LogicalDevice&&) noexcept;

		LogicalDevice& operator=(const LogicalDevice&) = delete;
		LogicalDevice& operator=(LogicalDevice&&) = default;

		VkDevice logicalDevice() const { return m_Device; }


	private:
		const Shared<PhysicalDevice> m_PhysicalDevice;
		const Shared<Instance> m_Instance;
		const std::vector<const char*>& m_DeviceExtensions;
		VkDevice m_Device{VK_NULL_HANDLE};

	private:
		void initLogicalDevice();
		void cleanup();
	};
}
