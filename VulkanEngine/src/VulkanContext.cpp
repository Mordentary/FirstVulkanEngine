#include "pch.h"
#include "VulkanContext.h"

#include "Application.h"
#include "QueueHandler.h"

namespace vkEngine
{
	VulkanContext::VulkanContext(const Engine& engine, const std::vector<const char*>& deviceExtensions)
		: m_Engine(engine)
	{
		initialize(deviceExtensions);
	}

	VulkanContext::~VulkanContext()
	{
		cleanup();
	}

	void VulkanContext::initialize(const std::vector<const char*>& deviceExtensions)
	{
		initPhysicalDevice(deviceExtensions);
		initLogicalDevice(deviceExtensions);
		initQueueHandler();
		initSwapchain();
	}

	void VulkanContext::initSwapchain()
	{
		m_Swapchain = CreateScoped<Swapchain>
			(
				m_Engine.getApp()->getWindow(),
				m_Engine.getApp()->getWindow()->getSurface(),
				*this,
				m_Engine.s_MaxFramesInFlight
			);
	}

	void VulkanContext::initQueueHandler()
	{
		m_QueueHandler = CreateScoped<QueueHandler>(*this);
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
	}

}