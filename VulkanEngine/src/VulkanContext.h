#pragma once

#include <unordered_set>
#include <optional>
#include "ValidationLayersManager.h"
#include "Window/Window.h"
#include "Devices/PhysicalDevice.h"
#include "Devices/LogicalDevice.h"
#include "CommandBufferHandler.h"

#include "Core.h"


namespace vkEngine
{
	class Engine;
	class Application;
	class QueueHandler;

	using QueueFamilyIndex = uint32_t;


	class VulkanContext
	{
		friend class Engine;


	public:
		static inline const Shared<PhysicalDevice>& getPhysicalDevice() { return m_ContextInstance->m_PhysicalDevice; };
		static inline const Shared<QueueHandler>& getQueueHandler() { return m_ContextInstance->m_QueueHandler; }
		static inline const Shared<Swapchain>& getSwapchain() { return m_ContextInstance->m_Swapchain; }
		static inline const Shared<LogicalDevice>& getLogicalDevice() { return m_ContextInstance->m_Device; };
		static inline const Shared<CommandBufferHandler>& getCommandHandler() { return m_ContextInstance->m_CommandHandler; };


		static inline VkDevice getDevice() { return m_ContextInstance->m_Device->logicalDevice(); }

		//TODO: Find the better place for functions
		VkFormat findDepthFormat();
		bool hasStencilComponent(VkFormat format);
	private:
		static inline void vulkanContextDeleterFunc(VulkanContext* ptr);
		using ScopedVulkanContext = Scoped<VulkanContext, decltype(&vulkanContextDeleterFunc)>;
	private:
		~VulkanContext();
		VulkanContext(const Engine& engine, const std::vector<const char*>& deviceExtensions);
		void initialize(const std::vector<const char*>& deviceExtensions);

		static void initializeInstance(const Engine& engine, const std::vector<const char*>& deviceExtensions);
		static void destroyInstance();
		void cleanup();

	private:
		static ScopedVulkanContext m_ContextInstance;
		const Engine& m_Engine;
		Shared<CommandBufferHandler> m_CommandHandler = nullptr;
		Shared<Swapchain> m_Swapchain = nullptr;
		Shared<QueueHandler> m_QueueHandler = nullptr;
		Shared<PhysicalDevice> m_PhysicalDevice = nullptr;
		Shared<LogicalDevice> m_Device = nullptr;
	private:
		inline void initCommandBufferHandler();
		inline void initSwapchain();
		inline void initQueueHandler();
		inline void initPhysicalDevice(const std::vector<const char*>& deviceExtensions);
		inline void initLogicalDevice(const std::vector<const char*>& deviceExtensions);

	};


}
