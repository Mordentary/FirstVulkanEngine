#include "pch.h"

#include "Engine.h"

#include "Application.h"
#include "QueueHandler.h"
#include "Images/Image2D.h"
#include "Utility/VulkanUtils.h"

namespace vkEngine
{
	static uint32_t nextRenderFrame = 0;
	const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const int WINDOW_STARTUP_HEIGHT = 1000, WINDOW_STARTUP_WIDTH = 1000;
	const std::string APP_NAME = "VulkanEngine";

	Engine::Engine(const Application* app)
		: m_App(app)
	{
		initInstance();
	};


	void vkEngine::Engine::run()
	{
		init();

		Timer timer("DeltaTimer");
		Timestep deltaTime(0.16f);

		while (!m_App->getWindow()->shouldClose())
		{
			timer.Start();
			deltaTime = timer.GetTimeSeconds();

			update(deltaTime);
			render();

			timer.Stop();
		}

		vkDeviceWaitIdle(VulkanContext::getLogicalDevice()->logicalDevice());
		cleanup();
	}

	void Engine::init()
	{
		VulkanContext::initializeInstance(*this, deviceExtensions);
		m_Camera = CreateShared<Camera>(glm::vec3{ 0.f, 0.5f, -1.f }, glm::vec3{ 0.f }, m_App->getWindow());
		initVulkan();
	}

	void Engine::initInstance()
	{
		m_Instance = CreateShared<Instance>(m_App->getAppName(), m_App->getValidationLayers(), m_App->isValidationLayersEnabled());
	}

	void vkEngine::Engine::initVulkan()
	{
		initRenderPass();
		initDescriptorsSetLayout();
		initGraphicsPipeline();
		VulkanContext::getSwapchain()->initFramebuffers(m_RenderPass); // TODO: Framebuffers are part of renderpass not swapchain. Not a good place for this.
		initCommandPool();
		initCommandBuffer();

		//m_TextureTest = CreateShared<Image2D>("assets/textures/statue.jpg");

		initTextureImage();
		initTextureImageView();
		initTextureSampler();

		initVertexBuffer();
		initIndexBuffer();
		initUniformBuffer();
		initDescriptorPool();
		initDescriptorSets();
		initSyncObjects();
	}

	void vkEngine::Engine::update(Timestep deltaTime)
	{
		m_Camera->Update(deltaTime);

		updateUniformBuffer(nextRenderFrame, deltaTime);

		m_App->getWindow()->pollEvents();
	}

	void Engine::render()
	{
		vkWaitForFences(VulkanContext::getLogicalDevice()->logicalDevice(), 1, &m_InFlightFences[nextRenderFrame], VK_TRUE, UINT64_MAX);

		auto& swapchain = VulkanContext::getSwapchain();
		VkResult result = swapchain->acquireNextImage(nextRenderFrame);
		uint32_t imageIndex = swapchain->getImageIndex();

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			swapchain->recreateSwapchain(m_RenderPass);
			return;
		}
		else
			ENGINE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image!");

		vkResetFences(VulkanContext::getLogicalDevice()->logicalDevice(), 1, &m_InFlightFences[nextRenderFrame]);

		vkResetCommandBuffer(m_CommandBuffers[nextRenderFrame], 0);
		recordCommandBuffer(m_CommandBuffers[nextRenderFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { swapchain->getImageSemaphore(nextRenderFrame) };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[nextRenderFrame];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[nextRenderFrame] };
		uint32_t signalSemaphoresCount = 1;
		submitInfo.signalSemaphoreCount = signalSemaphoresCount;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VulkanContext::getQueueHandler()->submitCommands(submitInfo, m_InFlightFences[nextRenderFrame]);

		swapchain->present(signalSemaphores, signalSemaphoresCount);

		nextRenderFrame = (imageIndex + 1) % s_MaxFramesInFlight;
	}

	void vkEngine::Engine::cleanup()
	{
		//TODO: swapchain deletion

		VkDevice device = VulkanContext::getLogicalDevice()->logicalDevice();
		m_TextureTest.reset();
		vkDestroySampler(device, m_TextureSampler, nullptr);

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			vkDestroyBuffer(device, m_UniformBuffers[i], nullptr);
			vkFreeMemory(device, m_UniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(device, m_DesciptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		vkDestroyBuffer(device, m_IndexBuffer, nullptr);
		vkFreeMemory(device, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(device, m_VertexBuffer, nullptr);
		vkFreeMemory(device, m_VertexBufferMemory, nullptr);

		vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		VulkanContext::destroyInstance();
	}




	VkPresentModeKHR Engine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& abailableModes)
	{
		for (const auto& availablePresentMode : abailableModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkSurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}
		return availableFormats[0];
	}

	VkExtent2D Engine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			ENGINE_INFO("Swapchain extent: %d, %d", capabilities.currentExtent.width, capabilities.currentExtent.height);
			return capabilities.currentExtent;
		}
		else
		{
			auto [width, height] = m_App->getWindow()->getWindowSize();

			ENGINE_INFO("GLFW Window size: %d, %d", width, height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void Engine::initDescriptorsSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		ENGINE_ASSERT(vkCreateDescriptorSetLayout(VulkanContext::getLogicalDevice()->logicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) == VK_SUCCESS, "Layout descriptors set creation failed");
	}

	void Engine::initDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(s_MaxFramesInFlight);

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(s_MaxFramesInFlight);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(s_MaxFramesInFlight);
		ENGINE_ASSERT(vkCreateDescriptorPool(VulkanContext::getLogicalDevice()->logicalDevice(), &poolInfo, nullptr, &m_DesciptorPool) == VK_SUCCESS, "Descriptor pool creation failed");
	}

	void Engine::initDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(s_MaxFramesInFlight, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DesciptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(s_MaxFramesInFlight);
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(s_MaxFramesInFlight);

		ENGINE_ASSERT(vkAllocateDescriptorSets(VulkanContext::getLogicalDevice()->logicalDevice(), &allocInfo, m_DescriptorSets.data()) == VK_SUCCESS, "Descriptor sets allocations failed");

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_TextureView;
			imageInfo.sampler = m_TextureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_DescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_DescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(VulkanContext::getLogicalDevice()->logicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void Engine::initUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(s_MaxFramesInFlight);
		m_UniformBuffersMemory.resize(s_MaxFramesInFlight);
		m_UniformBuffersMapped.resize(s_MaxFramesInFlight);

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			initBuffer
			(
				bufferSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				m_UniformBuffers[i],
				m_UniformBuffersMemory[i]
			);

			vkMapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
		}
	}

	void Engine::initTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanContext::getPhysicalDevice()->physicalDevice(), &properties);

		if (VulkanContext::getPhysicalDevice()->getFeatures().samplerAnisotropy)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		}
		else
		{
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy = 1.0f;
		}

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		ENGINE_ASSERT(vkCreateSampler(VulkanContext::getLogicalDevice()->logicalDevice(), &samplerInfo, nullptr, &m_TextureSampler) == VK_SUCCESS, "Create sampler failed");
	}

	void Engine::initDepthResources()
	{
	}

	VkImageView Engine::createImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		ENGINE_ASSERT(vkCreateImageView(VulkanContext::getLogicalDevice()->logicalDevice(), &viewInfo, nullptr, &imageView) == VK_SUCCESS, "ImageView creation failed");

		return imageView;
	}

	void Engine::initTextureImageView()
	{
		m_TextureView = m_TextureTest->getImageView();
	}

	void Engine::initTextureImage()
	{
		std::string path = "assets/textures/statue.jpg";
		int texWidth, texHeight, texChannels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * (texChannels < 3 ? texChannels : 4);

		ENGINE_ASSERT(pixels, (std::string("Failed to load image!") + " STB_IMAGE_FAILURE_REASON: " + stbi_failure_reason() + "! Path: " + path).c_str());

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		initBuffer
		(
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory
		);

		void* data;
		vkMapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory);

		stbi_image_free(pixels);

		VkFormat format{};
		if (texChannels == 4)
			format = VK_FORMAT_R8G8B8A8_SRGB;
		else if (texChannels == 3)
			format = VK_FORMAT_R8G8B8A8_SRGB;
		else if (texChannels == 2)
			format = VK_FORMAT_R8G8_SRGB;
		else if (texChannels == 1)
			format = VK_FORMAT_R8_SRGB;
		else
			ENGINE_ASSERT(false, "Format is not specified");


		m_TextureTest = CreateShared<Image2D>(
			VkExtent2D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) }, format,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


		//initImage(
		//	texWidth, texHeight, format,
		//	VK_IMAGE_TILING_OPTIMAL,
		//	VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		//	m_Texture, m_TextureMemory);


		VkCommandBuffer cmdBuffer = VulkanUtils::beginSingleTimeCommands(m_CommandPool);

		m_TextureTest->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_TextureTest->copyBufferToImage(cmdBuffer, stagingBuffer, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		m_TextureTest->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VulkanUtils::endSingleTimeCommands(m_CommandPool, cmdBuffer);

		m_Texture = m_TextureTest->getImage();

		vkDestroyBuffer(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, nullptr);
	}

	//void Engine::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	//{
	//	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	//	VkBufferImageCopy region{};
	//	region.bufferOffset = 0;
	//	region.bufferRowLength = 0;
	//	region.bufferImageHeight = 0;

	//	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	region.imageSubresource.mipLevel = 0;
	//	region.imageSubresource.baseArrayLayer = 0;
	//	region.imageSubresource.layerCount = 1;

	//	region.imageOffset = { 0, 0, 0 };
	//	region.imageExtent = {
	//		width,
	//		height,
	//		1
	//	};

	//	vkCmdCopyBufferToImage(
	//		commandBuffer,
	//		buffer,
	//		image,
	//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//		1,
	//		&region
	//	);

	//	VulkanUtils::endSingleTimeCommands(commandBuffer);
	//}

	void Engine::initImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateImage(VulkanContext::getLogicalDevice()->logicalDevice(), &imageInfo, nullptr, &image) == VK_SUCCESS, "Image creation failed");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(VulkanContext::getLogicalDevice()->logicalDevice(), image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties);

		ENGINE_ASSERT(vkAllocateMemory(VulkanContext::getLogicalDevice()->logicalDevice(), &allocInfo, nullptr, &imageMemory) == VK_SUCCESS, "Failed to allocate image memory");

		vkBindImageMemory(VulkanContext::getLogicalDevice()->logicalDevice(), image, imageMemory, 0);
	}

	void Engine::updateUniformBuffer(uint32_t currentFrame, Timestep deltaTime)
	{
		UniformBufferObject ubo{};

		ubo.modelMat = glm::mat4(1.0f); // glm::rotate(glm::mat4(1.0f), cos(CurrentTime::GetCurrentTimeInSec()) * glm::radians(60.0f) * 10.f, glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.viewMat = m_Camera->GetViewMatrix();
		ubo.projMat = m_Camera->GetProjectionMatrix();
		ubo.projMat[1][1] *= -1;

		memcpy(m_UniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
	}

	void Engine::initGraphicsPipeline()
	{
		auto vertShaderCode = readFile("shaders/bin/defaultShader.vert.spv");
		auto fragShaderCode = readFile("shaders/bin/defaultShader.frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr; // possible constant values

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		fragShaderStageInfo.pSpecializationInfo = nullptr; // possible constant values

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkExtent2D swapchainExtent = VulkanContext::getSwapchain()->getExtent();
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // Discarding or clamping vertices that are outside of planes. Needs a GPU feature enabled
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // Disables geometry ability to pass through rasterizer stage(basically disables output)
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // How fragments are generated
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE; // Culling side
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE; // Bias for shadow mapping

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.depthTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		ENGINE_ASSERT(vkCreatePipelineLayout(VulkanContext::getLogicalDevice()->logicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS, "Pipeline layout creation failed");

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		ENGINE_ASSERT(vkCreateGraphicsPipelines(VulkanContext::getLogicalDevice()->logicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, "Pipeline creation failed");

		vkDestroyShaderModule(VulkanContext::getLogicalDevice()->logicalDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(VulkanContext::getLogicalDevice()->logicalDevice(), vertShaderModule, nullptr);
	}

	std::vector<char> Engine::readFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		ENGINE_ASSERT(file.is_open(), (std::string("Failed to open file at this path: ") + filename).c_str());

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule Engine::createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo moduleInfo{};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		moduleInfo.codeSize = code.size();

		VkShaderModule shaderModule{};
		ENGINE_ASSERT(vkCreateShaderModule(VulkanContext::getLogicalDevice()->logicalDevice(), &moduleInfo, nullptr, &shaderModule) == VK_SUCCESS, "Shader module creation failed")
			return shaderModule;
	}

	void Engine::initRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VulkanContext::getSwapchain()->getImagesFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; //what to do with framebuffer before rendering
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;//after

		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		ENGINE_ASSERT(vkCreateRenderPass(VulkanContext::getLogicalDevice()->logicalDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS, "Render pass creation failed");
	}

	void Engine::initCommandPool()
	{
		ENGINE_ASSERT(VulkanContext::getQueueHandler()->isGraphicsQueueSupported(), "Graphics queue family doesn't exist");

		QueueFamilyIndex graphicsFamilyIndex = VulkanContext::getQueueHandler()->getQueueFamilyIndices().graphicsFamily.value();

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		commandPoolInfo.queueFamilyIndex = (uint32_t)graphicsFamilyIndex;

		ENGINE_ASSERT(vkCreateCommandPool(VulkanContext::getLogicalDevice()->logicalDevice(), &commandPoolInfo, nullptr, &m_CommandPool) == VK_SUCCESS, "Command pool creation failed");
	}

	void Engine::initCommandBuffer()
	{
		m_CommandBuffers.resize(s_MaxFramesInFlight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		ENGINE_ASSERT(vkAllocateCommandBuffers(VulkanContext::getLogicalDevice()->logicalDevice(), &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS, "Command buffer allocation failed")
	}

	//void Engine::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	//{
	//	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	//	VkImageMemoryBarrier barrier{};
	//	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	//	barrier.oldLayout = oldLayout;
	//	barrier.newLayout = newLayout;
	//	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	//	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	//	barrier.image = image;
	//	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	//	barrier.subresourceRange.baseMipLevel = 0;
	//	barrier.subresourceRange.levelCount = 1;
	//	barrier.subresourceRange.baseArrayLayer = 0;
	//	barrier.subresourceRange.layerCount = 1;

	//	VkPipelineStageFlags sourceStage;
	//	VkPipelineStageFlags destinationStage;

	//	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	//	{
	//		barrier.srcAccessMask = 0;
	//		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	//		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	//		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//	}
	//	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	//	{
	//		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	//		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	//		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	//		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	//	}
	//	else
	//		ENGINE_ASSERT(false, "Layout transition is not supported")

	//		vkCmdPipelineBarrier(
	//			commandBuffer,
	//			sourceStage, destinationStage,
	//			0,
	//			0, nullptr,
	//			0, nullptr,
	//			1, &barrier
	//		);

	//	VulkanUtils::endSingleTimeCommands(commandBuffer);
	//}

	void Engine::initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateBuffer(VulkanContext::getLogicalDevice()->logicalDevice(), &bufferInfo, nullptr, &buffer) == VK_SUCCESS, "Buffer creation failed");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VulkanContext::getLogicalDevice()->logicalDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties);

		ENGINE_ASSERT(vkAllocateMemory(VulkanContext::getLogicalDevice()->logicalDevice(), &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Memory allocation failed");

		vkBindBufferMemory(VulkanContext::getLogicalDevice()->logicalDevice(), buffer, bufferMemory, 0);
	}

	void Engine::initVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory);

		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

		copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, nullptr);
	}

	void Engine::initIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory);

		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

		copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBuffer, nullptr);
		vkFreeMemory(VulkanContext::getLogicalDevice()->logicalDevice(), stagingBufferMemory, nullptr);
	}

	void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(m_CommandPool);
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		VulkanUtils::endSingleTimeCommands(m_CommandPool, commandBuffer);
	}

	void Engine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		ENGINE_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "Beginning of command buffer failed");

		VkExtent2D swapchainExtent = VulkanContext::getSwapchain()->getExtent();

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = VulkanContext::getSwapchain()->getFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;

		//VkClearValue clearColor = { {{0.850f, 0.796f, 0.937f, 1.0f}} };
		VkClearValue clearColor = { {{0.f, 0.f, 0.f, 1.0f}} };

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); //Last parameter is about execution of primary buffers

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline); // Second parameter is about pipeline how it will be used

		VkBuffer vertexBuffers[] = { m_VertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets
		(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_PipelineLayout,
			0,
			1,
			&m_DescriptorSets[nextRenderFrame],
			0,
			nullptr
		);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		ENGINE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Ending of command buffer failed");
	}

	void Engine::initSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_RenderFinishedSemaphores.resize(s_MaxFramesInFlight);
		m_InFlightFences.resize(s_MaxFramesInFlight);

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			ENGINE_ASSERT
			(
				vkCreateSemaphore(VulkanContext::getLogicalDevice()->logicalDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(VulkanContext::getLogicalDevice()->logicalDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, "Sync objects creation failed"
			);
		}
	}
}