#pragma once
#include "Engine.h"

namespace vkEngine
{
	class Engine;
	class Application
	{

	public:
		Application(bool enableLayers, uint32_t width, uint32_t height, const std::string& appName);
		~Application();
		void run();

		bool isValidationLayersEnabled() const { return m_EnableValidationLayers; };
		std::vector<const char*> getActivatedValidationLayers() const { return m_ValidationLayersManager->getValidationLayers(); };
		const Shared<Window> getWindow() const { return m_Window; };
		VkInstance getInstance() const { return m_Instance; };
		VkSurfaceKHR getSurface() const { return m_Surface; }
	private:

	private:
		void prepareEngine();
		void initInstance();
		void cleanup();
		bool extensionsAvailability(const char** requiredExt, uint32_t extensNum);
		std::vector<const char*> getRequiredExtensions();
	private:
		Shared<Engine> m_Engine = nullptr;
		Shared<Window> m_Window = nullptr;
		
		VkInstance m_Instance = nullptr;
		VkSurfaceKHR m_Surface = nullptr;
		Scoped<ValidationLayersManager> m_ValidationLayersManager = nullptr;
		const std::string m_AppName{};
		bool m_EnableValidationLayers;


	};
}
