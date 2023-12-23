#pragma once

#include <optional>
#include <array>
#include <glm/glm.hpp>

#include "TimeHelper.h"
#include "VulkanContext.h"
#include "Camera/Camera.h"
#include <Image2D.h>

namespace vkEngine
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 textureCoord;
		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices =
	{
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
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

		Engine(const Application* app);
		void run();
		const Application* getApp() const { return m_App; };
	public:
		static const uint32_t s_MaxFramesInFlight = 3;
		VkRenderPass m_RenderPass{ VK_NULL_HANDLE };
		const Shared<Instance>& getInstance() const { return m_Instance; };
	private:
		void update(Timestep deltaTime);
		void render();
		void cleanup();
		void init();
		void initInstance();


	private:
		const Application* m_App;
		Shared<Camera> m_Camera;
		Shared<Instance> m_Instance;

	private:
		void initVulkan();
		
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		void initDescriptorsSetLayout();
		void initDescriptorPool();
		void initDescriptorSets();

		void initGraphicsPipeline();
		static std::vector<char> readFile(const std::string& filename);
		VkShaderModule createShaderModule(const std::vector<char>& code);

		void initRenderPass();

		void initCommandPool();
		void initCommandBuffer();

		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);

		void initImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		VkImageView createImageView(VkImage image, VkFormat format);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		void initVertexBuffer();
		void initIndexBuffer();
		void initUniformBuffer();
		void initTextureImage();
		void initTextureImageView();
		void initTextureSampler();

		void initDepthResources();

		void updateUniformBuffer(uint32_t currentFrame, Timestep deltaTime);

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void initSyncObjects();
	private:

		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DesciptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

		VkBuffer m_VertexBuffer;
		VkDeviceMemory m_VertexBufferMemory;

		VkBuffer m_IndexBuffer;
		VkDeviceMemory m_IndexBufferMemory;


		//Image2D m_Texture;
		
		//TODO: replace with Image2D class
		VkSampler m_TextureSampler;
		VkImageView m_TextureView;
		VkImage m_Texture;
		VkDeviceMemory m_TextureMemory;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;

		std::vector<VkBuffer>  m_UniformBuffers{};
		std::vector<VkDeviceMemory> m_UniformBuffersMemory{};
		std::vector<void*> m_UniformBuffersMapped{};

		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers{};

		std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
		std::vector<VkFence> m_InFlightFences{};
	};
}
