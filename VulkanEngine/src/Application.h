#pragma once
#include "Engine.h"

namespace vkEngine
{
	class Engine;
	class Instance;

	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};
	
	class Application

	{
	public:
		Application(bool enableLayers, uint32_t width, uint32_t height, const std::string & appName);
		~Application();
		void run();

		const Shared<Window> getWindow() const { return m_Window; };
		const std::string& getAppName() const { return m_AppName; };
		const bool isValidationLayersEnabled() const { return m_ValidationLayersEnabled; };
		const std::vector<const char*>& getValidationLayers() const { return validationLayers; };

	private:
		void prepareEngine();
		void cleanup();
	private:
		Shared<Engine> m_Engine = nullptr;
		Shared<Window> m_Window = nullptr;
		const std::string m_AppName{};
		const bool m_ValidationLayersEnabled = false;

	};
}
