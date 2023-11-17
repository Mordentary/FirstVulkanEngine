#pragma once
#pragma once

#include <vector>
#include <string>
#include"ValidationLayersManager.h"

namespace vkEngine
{
	class Instance {
	public:
		Instance(const std::string& appName, const std::vector<const char*>& validationLayers, bool enableValidationLayers);
		~Instance();

		Instance(const Instance&) = delete;
		Instance(Instance&&) noexcept;

		Instance& operator=(const Instance&) = delete;
		Instance& operator=(Instance&&) = default;

		VkInstance instance() const;
		std::vector<const char*> getActivatedValidationLayers() const { return m_ValidationLayersManager->getValidationLayers(); };
		bool isValidationLayersEnabled() const { return m_EnableValidationLayers; };

	private:
		std::vector<const char*> getRequiredExtensions();
		bool extensionsAvailability(const char** requiredExt, uint32_t extensNum);
		void createInstance(const std::string& appName, bool enableValidationLayers);
		void cleanup();

	private:
		Scoped<ValidationLayersManager> m_ValidationLayersManager = nullptr;
		VkInstance m_Instance;
		std::vector<const char*> m_Extensions;
		bool m_EnableValidationLayers;
	};
}
