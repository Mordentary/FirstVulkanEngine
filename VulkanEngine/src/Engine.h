#pragma once

#include <optional>
#include <array>
#include <glm/glm.hpp>

#include "TimeHelper.h"
#include "Camera/Camera.h"
#include "Images/Image2D.h"
#include "Buffers/Buffer.h"
#include "Buffers/UniformBuffer.h"

namespace vkEngine
{

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

		void initImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		VkImageView createImageView(VkImage image, VkFormat format);

		
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

		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void initSyncObjects();
	private:

		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DesciptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;

		VkPipeline m_GraphicsPipeline;
		VkPipelineLayout m_PipelineLayout{ VK_NULL_HANDLE };

		Shared<Image2D> m_TextureTest{ nullptr};
		Scoped<VertexBuffer> m_VertexBuffer{ nullptr };
		Scoped<IndexBuffer> m_IndexBuffer{ nullptr };

		std::vector<Shared<UniformBuffer>> m_UniformBuffers{};


		//TODO: replace with Image2D class
		VkSampler m_TextureSampler;
		VkImageView m_TextureView;
		VkImage m_Texture;
		VkDeviceMemory m_TextureMemory;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;


		//std::vector<VkBuffer>  m_UniformBuffers{};
		//std::vector<VkDeviceMemory> m_UniformBuffersMemory{};
		//std::vector<void*> m_UniformBuffersMapped{};

		std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
		std::vector<VkFence> m_InFlightFences{};
	};
}
