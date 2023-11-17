#include "pch.h"
#include "Instance.h"
#include "GLFW/glfw3.h"

namespace vkEngine
{
	Instance::Instance(const std::string& appName, const std::vector<const char*>& validationLayers, bool enableValidationLayers)
		:
		m_Instance(VK_NULL_HANDLE),
		m_EnableValidationLayers(enableValidationLayers)
	{
		if (m_EnableValidationLayers && validationLayers.size() > 0)
		{
			m_ValidationLayersManager = CreateScoped<ValidationLayersManager>(validationLayers);
		}
		else
			ENGINE_INFO("Validation layers do not work");

		createInstance(appName, enableValidationLayers);
		m_ValidationLayersManager->setupDebugMessanger(m_Instance);
	}

	Instance::~Instance()
	{
		cleanup();
	}

	Instance::Instance(Instance&& other) noexcept
	{
		m_Instance = std::exchange(other.m_Instance, nullptr);
	}

	VkInstance Instance::instance() const
	{
		return m_Instance;
	}

	std::vector<const char*> Instance::getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (m_EnableValidationLayers)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}


	bool Instance::extensionsAvailability(const char** requiredExt, uint32_t extensNum)
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

	void Instance::createInstance(const std::string& appName, bool enableValidationLayers) {

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = appName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "CRYingeEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instInfo{};
		instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instInfo.pApplicationInfo = &appInfo;

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		if (enableValidationLayers)
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

	void Instance::cleanup()
	{
		m_ValidationLayersManager->destroyDebugMessenger(m_Instance);
		ENGINE_ASSERT(m_Instance != VK_NULL_HANDLE, "Instance is null in the destructor");
		vkDestroyInstance(m_Instance, nullptr);
	}


}
