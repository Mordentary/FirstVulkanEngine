#include "Application.h"

namespace vkEngine
{
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	Application::Application(bool enableLayers, uint32_t width, uint32_t height, const std::string& appName)
		: m_EnableValidationLayers(enableLayers), m_AppName(appName)
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
		if (m_EnableValidationLayers && validationLayers.size() > 0)
		{
			m_ValidationLayersManager = CreateScoped<ValidationLayersManager>(validationLayers);
		}
		else
			ENGINE_INFO("Validation layers do not work");

		initInstance();
		m_ValidationLayersManager->setupDebugMessanger(m_Instance);
		m_Window->createSurface(m_Instance, m_Surface);
		m_Engine = CreateShared<Engine>(this);
	}
	void Application::initInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "VulkanDemo";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "CringeEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instInfo{};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};


		if (m_EnableValidationLayers)
		{
			auto& validationLayers = m_ValidationLayersManager->getValidationLayers();
			instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());

			instInfo.ppEnabledLayerNames = validationLayers.data();

			m_ValidationLayersManager->populateDebugMessengerCreateInfo(debugCreateInfo);
			instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			instInfo.enabledLayerCount = 0;
			instInfo.ppEnabledLayerNames = nullptr;
			instInfo.pNext = nullptr;
		}


		auto appExntesions = getRequiredExtensions();

		ENGINE_ASSERT(extensionsAvailability(appExntesions.data(), (uint32_t)appExntesions.size() == false), "Extensions are unavailable");

		instInfo.enabledExtensionCount = static_cast<uint32_t>(appExntesions.size());
		instInfo.ppEnabledExtensionNames = appExntesions.data();

		ENGINE_ASSERT(vkCreateInstance(&instInfo, nullptr, &m_Instance) == VK_SUCCESS, "Instace creation failed");
	}

	bool Application::extensionsAvailability(const char** requiredExt, uint32_t extensNum)
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> avaibleExtensions(extensionCount);

		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, avaibleExtensions.data());

		for (size_t i = 0; i < extensNum; i++)
		{
			bool exists = false;
			for (const auto& ext : avaibleExtensions)
			{
				if (strcmp(requiredExt[i], ext.extensionName))
					exists = true;
			}
			if (!exists) return false;
		}

		return true;
	}

	std::vector<const char*> Application::getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_EnableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	SwapChainSupportDetails Application::querySwapChainSupport(VkPhysicalDevice device) const
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
		}

		uint32_t presentMode;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentMode, nullptr);
		if (presentMode != 0)
		{
			details.presentModes.resize(presentMode);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentMode, details.presentModes.data());
		}

		return details;
	}
	void Application::cleanup()
	{
		if (m_EnableValidationLayers && validationLayers.size() > 0)
			m_ValidationLayersManager->destroyDebugMessenger(m_Instance);

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		m_Window.reset();
	}
}
