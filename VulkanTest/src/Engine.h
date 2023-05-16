#pragma once

#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<vector>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
namespace vkEngine
{
	class Engine
	{
	public:
		void run();
	private:
		void update();
		void render();
		void cleanup();
		void initVulkan();
		void initWindow();

	private:
		GLFWwindow* m_Window = nullptr;
		uint32_t m_WindowHeight = 1000, m_WindowWidth = 1600;

	private:
		void initInstance();
		bool extensionsAvailability(const char** extensions, uint32_t extensNum);
		std::vector<const char*>  getRequiredExtensions();

	private: // Validation layers
		bool checkValidationLayerSupport();
		void setupDebugMessanger();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
		(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);
		VkResult createDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger);
		void destroyDebugUtilsMessengerEXT(VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	private:
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
	};
}
