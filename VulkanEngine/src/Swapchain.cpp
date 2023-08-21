#include "Swapchain.h"
#include "VulkanContext.h"
#include "logger/Logger.h"
#include "QueueHandler.h"
#include "window/Window.h"

namespace vkEngine
{

	Swapchain::Swapchain(const Shared<Window>& window, VkSurfaceKHR surface, const VulkanContext& context, uint32_t maxFramesInFlight)
		: m_Context(context), m_Window(window), m_Surface(surface), m_Device(context.getDevice()), m_MaxFramesInFlight(maxFramesInFlight)
	{
		initSwapchain();
		initImageViews();
		initSemaphores();
	}

	void Swapchain::initImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			//TODO: static function in texture class

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapchainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			ENGINE_ASSERT(vkCreateImageView(m_Context.getDevice(), &viewInfo, nullptr, &m_SwapchainImageViews[i]) == VK_SUCCESS, "ImageView creation failed");

		}
	}

	void Swapchain::recreateSwapchain(VkRenderPass renderpass)
	{
		std::pair<int, int> screenSize{};
		screenSize = m_Window->getWindowSize();

		while (screenSize.first == 0 || screenSize.second == 0)
		{
			screenSize = m_Window->getWindowSize();
			m_Window->waitEvents();
		}

		vkDeviceWaitIdle(m_Context.getDevice());

		cleanupSwapchain();

		initSwapchain();
		initImageViews();
		initFramebuffers(renderpass);
	}

	void Swapchain::cleanupSwapchain()
	{
		for (size_t i = 0; i < m_SwapchainFramebuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_Context.getDevice(), m_SwapchainFramebuffers[i], nullptr);
		}

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Context.getDevice(), m_SwapchainImageViews[i], nullptr);
			vkDestroySemaphore(m_Context.getDevice(), m_ImageAvailableSemaphores[i], nullptr);
		}

		vkDestroySwapchainKHR(m_Context.getDevice(), m_Swapchain, nullptr);
	}

	void Swapchain::initFramebuffers(VkRenderPass renderpass)
	{
		m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageView attachments[] =
			{
			  m_SwapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderpass; //TODO: renderpass class?
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &attachments[0];
			framebufferInfo.width = m_SwapchainExtent.width;
			framebufferInfo.height = m_SwapchainExtent.height;
			framebufferInfo.layers = 1;

			ENGINE_ASSERT(vkCreateFramebuffer(m_Context.getDevice(), &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) == VK_SUCCESS, "Framebuffer creation failed");
		}
	}
	void Swapchain::initSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);

		for (size_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			ENGINE_ASSERT
			(
				vkCreateSemaphore(m_Context.getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS, "Semaphore creation failed"
			);
		}
	}
	VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes)
	{
		for (const auto& availablePresentMode : abailableModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}
		return availableFormats[0];
	}

	VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			ENGINE_INFO("Swapchain extent: %d, %d", capabilities.currentExtent.width, capabilities.currentExtent.height);
			return capabilities.currentExtent;
		}
		else
		{
			auto [width, height] = m_Window->getWindowSize();

			ENGINE_INFO("GLFW Window size: %d, %d", width, height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void Swapchain::initSwapchain()
	{
		SwapChainSupportDetails swapChainSupport = m_Context.querySwapChainSupport(m_Context.getPhysicalDevice());

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_Surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = m_Context.getQueueHandler()->getQueueFamilyIndices();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //explicit ownership and which queues will be able to take ownership
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
			swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional;
		}

		swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Correspons for alpha blending with other windows
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		ENGINE_ASSERT(vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &m_Swapchain) == VK_SUCCESS, "Swapchain creation failed");

		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
	}

	void Swapchain::resize(uint32_t newWidth, uint32_t newHeight)
	{
	}

	VkResult Swapchain::acquireNextImage(uint32_t frame)
	{
		return vkAcquireNextImageKHR(m_Context.getDevice(), m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[frame], VK_NULL_HANDLE, &m_ImageIndex);
	}

	void Swapchain::present(VkSemaphore* signalSemaphores, uint32_t count)
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = count;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_Swapchain };

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_ImageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(m_Context.getQueueHandler()->getGraphicsQueue(), &presentInfo);
	}
}