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
		Swapchain() = default;
		Swapchain(VkPhysicalDevice device, uint32_t width, uint32_t height);
		~Swapchain();

		void resize(uint32_t newWidth, uint32_t newHeight);
		void acquireNextImage();
		void submitAndPresent();

	private:
		void init(VkPhysicalDevice device);

		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D chooseSwapExtent(const Shared<Window>& window, const VkSurfaceCapabilitiesKHR& capabilities);

	private:
		VkSwapchainKHR m_Swapchain{ nullptr };
		std::vector<VkImage> m_SwapchainImages{};
		std::vector<VkImageView> m_SwapchainImageViews{};
		SwapChainSupportDetails m_SwapchainDetails{};
		VkExtent2D m_SwapchainExtent{};
		uint32_t m_Width = 1000, m_Height = 1000;

	};

}
