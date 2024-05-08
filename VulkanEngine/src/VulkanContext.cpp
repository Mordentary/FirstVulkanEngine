#include "pch.h"
#include "VulkanContext.h"

#include "Application.h"
#include "QueueHandler.h"

namespace vkEngine
{
	VulkanContext::ScopedVulkanContext VulkanContext::m_ContextInstance(nullptr, &VulkanContext::vulkanContextDeleterFunc);

	void VulkanContext::vulkanContextDeleterFunc(VulkanContext* ptr)
	{
		delete ptr;
	}

	void VulkanContext::initializeInstance(const Engine& engine, const std::vector<const char*>& deviceExtensions)
	{
		m_ContextInstance = ScopedVulkanContext(new VulkanContext(engine, deviceExtensions), &vulkanContextDeleterFunc);
	}

	VulkanContext::VulkanContext(const Engine& engine, const std::vector<const char*>& deviceExtensions)
		: m_Engine(engine)
	{
		initialize(deviceExtensions);
	}

	VulkanContext::~VulkanContext()
	{
		cleanup();
	}


	void VulkanContext::destroyInstance()
	{
		m_ContextInstance.reset();
	}

	void VulkanContext::initialize(const std::vector<const char*>& deviceExtensions)
	{
		initPhysicalDevice(deviceExtensions);
		initLogicalDevice(deviceExtensions);
		initQueueHandler();
		initSwapchain();
		initCommandBufferHandler();

	}

	inline void VulkanContext::initCommandBufferHandler()
	{
		m_CommandHandler = CreateShared<CommandBufferHandler>(m_Device->logicalDevice(), m_QueueHandler->getQueueFamilyIndices().graphicsFamily.value());
	}

	void VulkanContext::initSwapchain()
	{
		m_Swapchain = CreateScoped<Swapchain>
			(
				m_Engine.getApp()->getWindow(),
				m_Engine.getApp()->getWindow()->getSurface(),
				m_Device,
				m_PhysicalDevice,
				m_QueueHandler,
				m_Engine.s_MaxFramesInFlight
			);
	}

	void VulkanContext::initQueueHandler()
	{
		m_QueueHandler = CreateScoped<QueueHandler>(m_Device, m_PhysicalDevice);
	}

	inline void VulkanContext::initPhysicalDevice(const std::vector<const char*>& deviceExtensions)
	{
		m_PhysicalDevice = CreateShared<PhysicalDevice>(m_Engine.getInstance(), m_Engine.getApp()->getWindow(), deviceExtensions);
	}

	inline void VulkanContext::initLogicalDevice(const std::vector<const char*>& deviceExtensions)
	{
		m_Device = CreateShared<LogicalDevice>(m_PhysicalDevice, m_Engine.getInstance(), deviceExtensions);
	}

	VkFormat VulkanContext::findDepthFormat()
	{
		return m_PhysicalDevice->findSupportedFormat(
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
		m_CommandHandler.reset();
	}


}