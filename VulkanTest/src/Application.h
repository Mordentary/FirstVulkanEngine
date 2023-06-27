#pragma once

#include <Window/Window.h>_
#include <ValidationLayersManager.h>
#include<../Core.h>

namespace vkEngine
{
	class Application
	{
	
	public:

		Application(bool enableLayers, const std::vector<const char*>& validationLayers, uint32_t width, uint32_t height, const std::string& appName);
		~Application();

		void initInstance();
		void cleanup();

		bool isValidationLayersEnabled() { return m_EnableValidationLayers; };
		std::vector<const char*> getActivatedValidationLayers() { return m_ValidationLayersManager->getValidationLayers(); };
		const Shared<Window> getWindow() const { return m_Window; };
		VkInstance getInstance() const { return m_Instance; };
		VkSurfaceKHR getSurface() { return m_Surface; }
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
	private:
		bool extensionsAvailability(const char** requiredExt, uint32_t extensNum);
		std::vector<const char*> getRequiredExtensions();
	private:
		VkInstance m_Instance{ nullptr };
		VkSurfaceKHR m_Surface{ nullptr };
		Shared<Window> m_Window = nullptr;
		Scoped<ValidationLayersManager> m_ValidationLayersManager = nullptr;
		const std::string m_AppName{};
		bool m_EnableValidationLayers;

	};
}
