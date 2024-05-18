#include "pch.h"

#include "Application.h"

namespace vkEngine
{

	Application::Application(bool enableLayers, uint32_t width, uint32_t height, const std::string& appName)
		:
		m_AppName(appName),
		m_ValidationLayersEnabled(enableLayers)
	{
		m_Window = CreateShared<Window>(width, height, m_AppName);
	}

	Application::~Application()
	{
		cleanup();
	}

	void Application::run()
	{
		prepareEngine();
		m_Engine->run();
	}

	void Application::prepareEngine()
	{
		m_Engine = CreateScoped<Engine>(this);
		m_Window->createSurface(m_Engine->getInstance());

	}

	void Application::cleanup()
	{
		m_Window->destroySurface(m_Engine->getInstance());
	}
}