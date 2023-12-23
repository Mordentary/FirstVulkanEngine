#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdint>
#include <vector>
#include <algorithm>
#include "Core.h"

namespace vkEngine
{
	class Window;
	class VulkanContext;
	class LogicalDevice;
	class QueueHandler;
	class PhysicalDevice;

	struct QueueFamilyIndices;

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};
	};

	class Swapchain
	{
		friend Window;

	public:
		Swapchain(const Shared<Window>& window, VkSurfaceKHR surface, Shared<LogicalDevice>& device, Shared<PhysicalDevice>& physicalD, Shared<QueueHandler>& qHandler, uint32_t maxFramesInFlight);
		Swapchain() = delete;

		void resize(uint32_t newWidth, uint32_t newHeight);
		VkResult acquireNextImage(uint32_t frame);
		void present(VkSemaphore* signalSemaphores, uint32_t count);
		void recreateSwapchain(VkRenderPass renderpass);
		void cleanupSwapchain();

		VkSemaphore getImageSemaphore(uint32_t frame) { return m_ImageAvailableSemaphores[frame]; }
		VkFormat getImagesFormat() const { return m_SwapchainImageFormat; }
		VkExtent2D getExtent() const { return m_SwapchainExtent; }
		VkFramebuffer getFramebuffer(uint32_t index) const { return m_SwapchainFramebuffers[index]; }
		void initFramebuffers(VkRenderPass renderpass);
		uint32_t getImageIndex() const { return m_ImageIndex; }

	private:
		void initSwapchain();
		void initImageViews();
		void initSemaphores();
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	private:
		const Shared<Window>& m_Window;
		const Shared<QueueHandler>& m_QueueHandlerRef;
		const Shared<LogicalDevice>& m_DeviceRef;
		const Shared<PhysicalDevice>& m_PhysicalDeviceRef;

		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_Swapchain{ nullptr };
		std::vector<VkSemaphore> m_ImageAvailableSemaphores{};

		std::vector<VkFramebuffer> m_SwapchainFramebuffers{};

		//TODO: should I go with different image class aka swapchainImage?
		std::vector<VkImage> m_SwapchainImages{};
		std::vector<VkImageView> m_SwapchainImageViews{};
		VkFormat m_SwapchainImageFormat{};
		VkExtent2D m_SwapchainExtent{};

		uint32_t m_ImageIndex;
		const uint32_t& m_MaxFramesInFlight;

		uint32_t m_Width = 1000, m_Height = 1000;
	};
}
