#pragma once

#include <unordered_set>
#include <optional>
#include "ValidationLayersManager.h"
#include "Window/Window.h"
#include "Devices/PhysicalDevice.h"
#include "Devices/LogicalDevice.h"

namespace vkEngine
{
	class Engine;
	class Application;
	class QueueHandler;
	using QueueFamilyIndex = uint32_t;

	class VulkanContext
	{
	public:
		VulkanContext(const Engine& engine, const std::vector<const char*>& deviceExtensions);
		~VulkanContext();

		inline const Shared<PhysicalDevice>& getPhysicalDevice() const { return m_PhysicalDevice; };
		inline const Shared<QueueHandler>& getQueueHandler() const { return m_QueueHandler; }
		inline const Shared<Swapchain>& getSwapchain() { return m_Swapchain; }
		inline const Shared<LogicalDevice>& getLogicalDevice() const { return m_Device; };
		inline VkDevice getDevice() const { return m_Device->logicalDevice(); }

		//TODO: Find the better place for functions
		VkFormat findDepthFormat();
		bool hasStencilComponent(VkFormat format);
	private:
		void cleanup();
		void initialize(const std::vector<const char*>& deviceExtensions);
	private:
		const Engine& m_Engine;
		Shared<Swapchain> m_Swapchain = nullptr;
		Shared<QueueHandler> m_QueueHandler = nullptr;
		Shared<PhysicalDevice> m_PhysicalDevice = nullptr;
		Shared<LogicalDevice> m_Device = nullptr;

	private:
		inline void initSwapchain();
		inline void initQueueHandler();
		inline void initPhysicalDevice(const std::vector<const char*>& deviceExtensions);
		inline void initLogicalDevice(const std::vector<const char*>& deviceExtensions);

	};
}
