#pragma once

#include <optional>
#include <array>
#include <glm/glm.hpp>

#include "TimeHelper.h"
#include "VulkanContext.h"
#include "Camera/Camera.h"



const int MAX_FRAMES_IN_FLIGHT = 2;


namespace vkEngine
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec3 color;
		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> vertices =
	{
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};
	const std::vector<uint16_t> indices =
	{
		0, 1, 2, 2, 3, 0
	};

	struct UniformBufferObject
	{
		glm::mat4 modelMat;
		glm::mat4 viewMat;
		glm::mat4 projMat;
	};


	class Application;
	class Engine
	{
	public:
		void run();
		
		const Application* getApp() const { return m_App; };
		Engine(const Application* app) : m_App(app){};
	
	private:
		void update(Timestep deltaTime);
		void render();
		void cleanup();
		void init();

	private:
		const Application* m_App;
		Shared<VulkanContext> m_Context;
		Shared<Camera> m_Camera;

	private:
		void initVulkan();
		void initSwapchain();
		void recreateSwapchain();
		void cleanupSwapChain();

		void initImageViews();

		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void initDescriptorsSetLayout();
		void initDescriptorPool();
		void initDescriptorSets();

		void initGraphicsPipeline();
		static std::vector<char> readFile(const std::string& filename);
		VkShaderModule createShaderModule(const std::vector<char>& code);

		void initFramebuffers();
		void initRenderPass();

		void initCommandPool();
		void initCommandBuffer();

		void initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void initVertexBuffer();
		void initIndexBuffer();
		void initUniformBuffer();
		void updateUniformBuffer(uint32_t currentFrame, Timestep deltaTime);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void initSyncObjects();
	private:

		VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
		std::vector<VkImage> m_SwapchainImages{};
		std::vector<VkImageView> m_SwapchainImageViews{};
		VkFormat m_SwapchainImageFormat;
		VkExtent2D m_SwapchainExtent{};



		std::vector<VkFramebuffer> m_SwapchainFramebuffers{};

		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DesciptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;

		VkPipeline m_GraphicsPipeline;
		VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
		VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

		VkBuffer m_VertexBuffer;
		VkDeviceMemory m_VertexBufferMemory;

		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;

		std::vector<VkBuffer>  m_UniformBuffers{};
		std::vector<VkDeviceMemory> m_UniformBuffersMemory{};
		std::vector<void*> m_UniformBuffersMapped{};

		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers{};

		std::vector<VkSemaphore> m_ImageAvailableSemaphores{};
		std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
		std::vector<VkFence> m_InFlightFences{};
	};
}
