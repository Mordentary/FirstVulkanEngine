#pragma once

#include<vector>
#include <vulkan/vulkan.h>

class VulkanContext;

namespace vkEngine
{
	class ValidationLayersManager
	{
	public:
		ValidationLayersManager() = default;
		ValidationLayersManager(const std::vector<const char*>& validationLayers);
		
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void destroyDebugMessenger(VkInstance instance);

		const std::vector<const char*>& getValidationLayers() const { return m_ValidationLayers; }
		
		void setupDebugMessanger(VkInstance instance);

	private:
		VkResult createDebugUtilsMessengerEXT(VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger);
	
		bool checkValidationLayerSupport();
		
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback
		(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData
		);
	private:
		std::vector<const char*> m_ValidationLayers;
		VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };
	};
}
