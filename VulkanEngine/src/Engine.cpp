#include "pch.h"

#include "Engine.h"

#include "Application.h"
#include "QueueHandler.h"
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

		vkDeviceWaitIdle(VulkanContext::getDevice());
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

	//void Engine::updateTextureDescriptor(Timestep deltaTime)
	//{
	//	m_TextureSwitchTimer += deltaTime.GetSeconds();

	//	if (m_TextureSwitchTimer >= 5.0f)
	//	{
	//		m_CurrentTextureIndex = (m_CurrentTextureIndex + 1) % m_Textures.size();

	//		VkDescriptorImageInfo imageInfo{};
	//		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//		imageInfo.imageView = m_Textures[m_CurrentTextureIndex]->getImageView();
	//		imageInfo.sampler = m_Textures[m_CurrentTextureIndex]->getSampler();

	//		for (size_t i = 0; i < m_DescriptorSets.size(); i++)
	//		{
	//			VkWriteDescriptorSet descriptorWrite{};
	//			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//			descriptorWrite.dstSet = m_DescriptorSets[i];
	//			descriptorWrite.dstBinding = 1; // Assumes texture is bound to binding 1
	//			descriptorWrite.dstArrayElement = 0;
	//			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	//			descriptorWrite.descriptorCount = 1;
	//			descriptorWrite.pImageInfo = &imageInfo;

	//			vkUpdateDescriptorSets(VulkanContext::getDevice(), 1, &descriptorWrite, 0, nullptr);
	//		}

	//		m_TextureSwitchTimer = 0.0f;
	//	}
	//}

	void vkEngine::Engine::initVulkan()
	{
		initRenderPass();
		initDescriptorsSetLayout();
		initGraphicsPipeline();
		VulkanContext::getSwapchain()->initFramebuffers(m_RenderPass); // TODO: Framebuffers are part of renderpass not swapchain. Not a good place for this.
		VulkanContext::getCommandHandler()->allocateCommandBuffers(s_MaxFramesInFlight);

		//m_TextureTest = CreateShared<Image2D>("assets/textures/statue.jpg");
		initTextureImage();
		//initTextureImageView();

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

		//updateTextureDescriptor(deltaTime);
		updateUniformBuffer(nextRenderFrame, deltaTime);

		m_App->getWindow()->pollEvents();
	}

	void Engine::render()
	{
		vkWaitForFences(VulkanContext::getDevice(), 1, &m_InFlightFences[nextRenderFrame], VK_TRUE, UINT64_MAX);

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

		vkResetFences(VulkanContext::getDevice(), 1, &m_InFlightFences[nextRenderFrame]);

		VkCommandBuffer cmdBuffer = VulkanContext::getCommandHandler()->getCommandBuffer(nextRenderFrame);
		vkResetCommandBuffer(cmdBuffer, 0);
		recordCommandBuffer(cmdBuffer, imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { swapchain->getImageSemaphore(nextRenderFrame) };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

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

		VkDevice device = VulkanContext::getDevice();
		m_TextureTest2.reset();

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			m_UniformBuffers[i].reset();
		}

		vkDestroyDescriptorPool(device, m_DesciptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);

		m_IndexBuffer.reset();
		m_VertexBuffer.reset();

		vkDestroyPipeline(device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

		vkDestroyRenderPass(device, m_RenderPass, nullptr);

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}

		//vkDestroyCommandPool(device, m_CommandPool, nullptr);

		VulkanContext::destroyInstance();
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


		ENGINE_ASSERT(vkCreateDescriptorSetLayout(VulkanContext::getDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) == VK_SUCCESS, "Layout descriptors set creation failed");
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

		ENGINE_ASSERT(vkCreateDescriptorPool(VulkanContext::getDevice(), &poolInfo, nullptr, &m_DesciptorPool) == VK_SUCCESS, "Descriptor pool creation failed");
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

		ENGINE_ASSERT(vkAllocateDescriptorSets(VulkanContext::getDevice(), &allocInfo, m_DescriptorSets.data()) == VK_SUCCESS, "Descriptor sets allocations failed");

		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i]->getBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = m_TextureTest2->getImageView();
			imageInfo.sampler = m_TextureTest2->getSampler();

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

			vkUpdateDescriptorSets(VulkanContext::getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void Engine::initUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(s_MaxFramesInFlight);
		for (size_t i = 0; i < s_MaxFramesInFlight; i++)
		{
			m_UniformBuffers[i] = CreateShared<UniformBuffer>(bufferSize);
			m_UniformBuffers[i]->mapMemory();
		}
	}


	void Engine::initTextureImage()
	{
		m_TextureTest2 = CreateShared<Texture2D>("assets/textures/brick_wall.jpg");
	}

	void Engine::updateUniformBuffer(uint32_t currentFrame, Timestep deltaTime)
	{
		UniformBufferObject ubo{};

		float time = CurrentTime::GetCurrentTimeInSec();
		ubo.modelMat = glm::rotate(glm::mat4(1.0f), glm::cos(time), glm::vec3(0.65, 0.52, 0.52)); // glm::rotate(glm::mat4(1.0f), cos(CurrentTime::GetCurrentTimeInSec()) * glm::radians(60.0f) * 10.f, glm::vec3(0.0f, 1.0f, 0.0f));

		ubo.viewMat = m_Camera->GetViewMatrix();
		ubo.projMat = m_Camera->GetProjectionMatrix();
		ubo.projMat[1][1] *= -1;

		memcpy(m_UniformBuffers[currentFrame]->getMappedMemory(), &ubo, sizeof(ubo));
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

		ENGINE_ASSERT(vkCreatePipelineLayout(VulkanContext::getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS, "Pipeline layout creation failed");

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

		ENGINE_ASSERT(vkCreateGraphicsPipelines(VulkanContext::getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, "Pipeline creation failed");

		vkDestroyShaderModule(VulkanContext::getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(VulkanContext::getDevice(), vertShaderModule, nullptr);
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
		ENGINE_ASSERT(vkCreateShaderModule(VulkanContext::getDevice(), &moduleInfo, nullptr, &shaderModule) == VK_SUCCESS, "Shader module creation failed")
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

		ENGINE_ASSERT(vkCreateRenderPass(VulkanContext::getDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS, "Render pass creation failed");
	}

	void Engine::initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateBuffer(VulkanContext::getDevice(), &bufferInfo, nullptr, &buffer) == VK_SUCCESS, "Buffer creation failed");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(VulkanContext::getDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = VulkanUtils::findMemoryType(memRequirements.memoryTypeBits, properties);

		ENGINE_ASSERT(vkAllocateMemory(VulkanContext::getDevice(), &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Memory allocation failed");

		vkBindBufferMemory(VulkanContext::getDevice(), buffer, bufferMemory, 0);
	}

	void Engine::initVertexBuffer()
	{
		m_VertexBuffer = CreateScoped<VertexBuffer>(vertices);
	}

	void Engine::initIndexBuffer()
	{
		m_IndexBuffer = CreateScoped<IndexBuffer>(indices);
	}

	void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = VulkanContext::getCommandHandler()->beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		VulkanContext::getCommandHandler()->endSingleTimeCommands(commandBuffer);
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

		VkBuffer vertexBuffers[] = { m_VertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);

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
				vkCreateSemaphore(VulkanContext::getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(VulkanContext::getDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, "Sync objects creation failed"
			);
		}
	}
}