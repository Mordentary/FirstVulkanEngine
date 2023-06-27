#include "Engine.h"

#include <Logger/Logger.h>
#include <set>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <glm/gtx/transform.hpp>
#include <chrono>

#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<glm/gtx/rotate_vector.hpp>
#include<glm/gtx/vector_angle.hpp>
#include "TimeHelper.h"

namespace vkEngine
{
	static uint32_t currentFrame = 0;
	const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const int WINDOW_STARTUP_HEIGHT = 1000, WINDOW_STARTUP_WIDTH = 1000;
	const std::string APP_NAME = "VulkanEngine";

	void vkEngine::Engine::run()
	{
		init();

		Timer timer("DeltaTimer");
		Timestep deltaTime(0.16);

		while (!m_App->getWindow()->shouldClose())
		{

			timer.Start();
			deltaTime = timer.GetTimeSeconds();

			update(deltaTime);
			render();


			timer.Stop();
		}

		vkDeviceWaitIdle(m_Context->getDevice());
		cleanup();
	}

	void Engine::init()
	{
		m_App = DEBUG_BUILD_CONFIGURATION ?
			CreateShared<Application>(true, validationLayers, WINDOW_STARTUP_WIDTH, WINDOW_STARTUP_HEIGHT, APP_NAME)
			:
			CreateShared<Application>(false, validationLayers, WINDOW_STARTUP_WIDTH, WINDOW_STARTUP_HEIGHT, APP_NAME);

		m_Context = CreateShared<VulkanContext>(m_App, deviceExtensions);

		m_Camera = CreateShared<Camera>(glm::vec3{ 1.f }, glm::vec3{ 0.f }, m_App->getWindow());
		
		initVulkan();
	}

	void vkEngine::Engine::initVulkan()
	{


		initSwapchain();
		initImageViews();


		initRenderPass();
		initDescriptorsSetLayout();
		initGraphicsPipeline();
		initFramebuffers();
		initCommandPool();
		initCommandBuffer();
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
	
		updateUniformBuffer(currentFrame, deltaTime);
		
		m_App->getWindow()->pollEvents();
	}

	void Engine::render()
	{
		vkWaitForFences(m_Context->getDevice(), 1, &m_InFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_Context->getDevice(), m_Swapchain, UINT64_MAX, m_ImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapchain();
			return;
		}
		else
			ENGINE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image!");

		vkResetFences(m_Context->getDevice(), 1, &m_InFlightFences[currentFrame]);

		vkResetCommandBuffer(m_CommandBuffers[currentFrame], 0);
		recordCommandBuffer(m_CommandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		ENGINE_ASSERT(vkQueueSubmit(m_Context->getGraphicsQueue(), 1, &submitInfo, m_InFlightFences[currentFrame]) == VK_SUCCESS, "Queue submition failed");

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_Swapchain };

		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(m_Context->getPresentQueue(), &presentInfo);


		currentFrame = (imageIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void vkEngine::Engine::cleanup()
	{
		cleanupSwapChain();

		VkDevice device = m_Context->getDevice();;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
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

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_InFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		m_Context.reset();
		m_App.reset();
	}


	//void Engine::initInstance()
	//{
	//	if (enableValidationLayers)
	//		ENGINE_ASSERT(checkValidationLayerSupport(), "Validation layers do not supported");

	//	VkApplicationInfo appInfo{};
	//	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	//	appInfo.pApplicationName = "VulkanDemo";
	//	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	//	appInfo.pEngineName = "CringeEngine";
	//	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	//	appInfo.apiVersion = VK_API_VERSION_1_0;

	//	VkInstanceCreateInfo instInfo{};
	//	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	//	instInfo.pApplicationInfo = &appInfo;

	//	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	//	if (enableValidationLayers)
	//	{
	//		instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	//		instInfo.ppEnabledLayerNames = validationLayers.data();

	//		populateDebugMessengerCreateInfo(debugCreateInfo);
	//		instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	//	}
	//	else
	//	{
	//		instInfo.enabledLayerCount = 0;
	//		instInfo.ppEnabledLayerNames = nullptr;
	//		instInfo.pNext = nullptr;
	//	}

	//	auto extensions = getRequiredExtensions();

	//	ENGINE_ASSERT(extensionsAvailability(extensions.data(), (uint32_t)extensions.size() == false), "Extensions are unavailable");

	//	instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	//	instInfo.ppEnabledExtensionNames = extensions.data();

	//	ENGINE_ASSERT(vkCreateInstance(&instInfo, nullptr, &m_Instance) == VK_SUCCESS, "Instace creation failed");
	//}

	//void Engine::pickPhysicalDevice()
	//{
	//	uint32_t deviceCount = 0;
	//	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
	//	ENGINE_ASSERT(deviceCount, "Failed to find GPUs with Vulkan support");

	//	std::vector<VkPhysicalDevice> devices(deviceCount);
	//	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

	//	deviceCount = 0;
	//	VkPhysicalDeviceProperties pickedDeviceProp;
	//	for (auto& device : devices)
	//	{
	//		vkGetPhysicalDeviceProperties(device, &pickedDeviceProp);
	//		if (VK_NULL_HANDLE == m_Context->getPhysicalDevice() && isDeviceSuitable(device))
	//		{
	//			m_Context->getPhysicalDevice() = device;
	//			ENGINE_INFO("--> Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);
	//		}
	//		else
	//			ENGINE_INFO("Device %" PRIu32 ": %s", deviceCount, pickedDeviceProp.deviceName);

	//		deviceCount++;
	//	}
	//	ENGINE_ASSERT(m_Context->getPhysicalDevice() != VK_NULL_HANDLE, "Failed to find suitable GPU");
	//}

	//void Engine::initLogicalDevice()
	//{
	//	QueueFamilyIndices indices = findQueueFamilies(m_Context->getPhysicalDevice());

	//	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	//	queueCreateInfos.reserve(indices.uniqueQueueFamilyCount());

	//	std::unordered_set<QueueFamilyIndex> uniqueIndices = indices.uniqueQueueFamilies();
	//	float queuePriority = 1.0f;
	//	for (QueueFamilyIndex family : uniqueIndices)
	//	{
	//		VkDeviceQueueCreateInfo queueCreateInfo{};
	//		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	//		queueCreateInfo.queueCount = 1;
	//		queueCreateInfo.queueFamilyIndex = family;
	//		queueCreateInfo.pQueuePriorities = &queuePriority;
	//		queueCreateInfos.push_back(queueCreateInfo);
	//	}

	//	VkPhysicalDeviceFeatures deviceFeatures{};

	//	VkDeviceCreateInfo deviceInfo{};
	//	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	//	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	//	deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	//	deviceInfo.pEnabledFeatures = &deviceFeatures;

	//	//Specify validation layers for older version of vulkan where is still the case (Previosly vulkan differ these two settings)
	//	if (enableValidationLayers)
	//	{
	//		deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	//		deviceInfo.ppEnabledLayerNames = validationLayers.data();
	//	}
	//	else {
	//		deviceInfo.enabledLayerCount = 0;
	//	}

	//	deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	//	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	//	ENGINE_ASSERT(vkCreateDevice(m_Context->getPhysicalDevice(), &deviceInfo, nullptr, &m_Context->getDevice()) == VK_SUCCESS, "Device creation failed");

	//	QueueFamilyIndex graphicsFamily = indices.graphicsFamily.value();
	//	QueueFamilyIndex presentFamily = indices.presentFamily.value();

	//	vkGetDeviceQueue(m_Context->getDevice(), graphicsFamily, 0, &m_GraphicsQueue);
	//	vkGetDeviceQueue(m_Context->getDevice(), presentFamily, 0, &m_PresentQueue);
	//}


	void Engine::initSwapchain()
	{
		SwapChainSupportDetails swapChainSupport = m_App->querySwapChainSupport(m_Context->getPhysicalDevice());

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		m_SwapchainImageFormat = surfaceFormat.format;
		m_SwapchainExtent = extent;

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_App->getSurface();
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = m_Context->getQueueIndices();
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //explicit ownership and which queues will be able to take ownership
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
			swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional;
		}

		swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Correspons for alpha blending with other windows
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		ENGINE_ASSERT(vkCreateSwapchainKHR(m_Context->getDevice(), &swapchainCreateInfo, nullptr, &m_Swapchain) == VK_SUCCESS, "Swapchain creation failed");

		vkGetSwapchainImagesKHR(m_Context->getDevice(), m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Context->getDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());
	}

	void Engine::recreateSwapchain()
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_App->getWindow()->getWindowGLFW(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(m_App->getWindow()->getWindowGLFW(), &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(m_Context->getDevice());

		cleanupSwapChain();

		initSwapchain();
		initImageViews();
		initFramebuffers();
	}

	void Engine::cleanupSwapChain()
	{
		for (size_t i = 0; i < m_SwapchainFramebuffers.size(); i++)
		{
			vkDestroyFramebuffer(m_Context->getDevice(), m_SwapchainFramebuffers[i], nullptr);
		}

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			vkDestroyImageView(m_Context->getDevice(), m_SwapchainImageViews[i], nullptr);
		}

		vkDestroySwapchainKHR(m_Context->getDevice(), m_Swapchain, nullptr);
	}

	void Engine::initImageViews()
	{
		m_SwapchainImageViews.resize(m_SwapchainImages.size());

		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_SwapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // specifies how image shoul be interpreted
			createInfo.format = m_SwapchainImageFormat;

			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // specifies how to swizzle the color channels around
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // describes mipmapping levels, layers and which part of image should be accessed
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			ENGINE_ASSERT(vkCreateImageView(m_Context->getDevice(), &createInfo, nullptr, &m_SwapchainImageViews[i]) == VK_SUCCESS, "ImageView creation failed");
		}
	}

	//bool Engine::isDeviceSuitable(VkPhysicalDevice device)
	//{
	//	VkPhysicalDeviceProperties deviceProperties;
	//	VkPhysicalDeviceFeatures deviceFeatures;
	//	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	//	QueueFamilyIndices indices = findQueueFamilies(device);

	//	bool extensSupported = checkDeviceExtensionSupport(device);
	//	bool swapChainAdequate = false;
	//	if (extensSupported) {
	//		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	//		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	//	}

	//	return
	//		indices.isComplete()
	//		&&
	//		extensSupported
	//		&&
	//		swapChainAdequate;
	//}

	//std::vector<const char*> Engine::getRequiredExtensions()
	//{
	//	uint32_t glfwExtensionCount = 0;
	//	const char** glfwExtensions;
	//	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	//	if (enableValidationLayers) {
	//		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	//	}

	//	return extensions;
	//}

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
			int width, height;
			glfwGetFramebufferSize(m_App->getWindow()->getWindowGLFW(), &width, &height);

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

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pBindings = &uboLayoutBinding;
		layoutInfo.bindingCount = 1;

		ENGINE_ASSERT(vkCreateDescriptorSetLayout(m_Context->getDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) == VK_SUCCESS, "Layout descriptors set creation failed");
	}

	void Engine::initDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		ENGINE_ASSERT(vkCreateDescriptorPool(m_Context->getDevice(), &poolInfo, nullptr, &m_DesciptorPool) == VK_SUCCESS, "Descriptor pool creation failed");

	}

	void Engine::initDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DesciptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		m_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

		ENGINE_ASSERT(vkAllocateDescriptorSets(m_Context->getDevice(), &allocInfo, m_DescriptorSets.data()) == VK_SUCCESS, "Descriptor sets allocations failed");


		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_UniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;

			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;

			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional

			vkUpdateDescriptorSets(m_Context->getDevice(), 1, &descriptorWrite, 0, nullptr);

		}
	}

	void Engine::initUniformBuffer()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_UniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			initBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);

			vkMapMemory(m_Context->getDevice(), m_UniformBuffersMemory[i], 0, bufferSize, 0, &m_UniformBuffersMapped[i]);
		}
	}

	void Engine::updateUniformBuffer(uint32_t currentFrame, Timestep deltaTime)
	{
		UniformBufferObject ubo{};


		static glm::vec3 position = glm::vec3(2.0f);

		ubo.modelMat = glm::rotate(glm::mat4(1.0f), deltaTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.viewMat = m_Camera->GetViewMatrix();
		ubo.projMat = m_Camera->GetProjectionMatrix();
		ubo.projMat[1][1] *= -1;


		memcpy(m_UniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
	}

	void Engine::initGraphicsPipeline()
	{
		auto vertShaderCode = readFile("shaders/bin/defaultVert.spv");
		auto fragShaderCode = readFile("shaders/bin/defaultFrag.spv");

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

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_SwapchainExtent.width;
		viewport.height = (float)m_SwapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapchainExtent;

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
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Culling side
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
		colorBlendAttachment.blendEnable = VK_TRUE;
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

		ENGINE_ASSERT(vkCreatePipelineLayout(m_Context->getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS, "Pipeline layout creation failed");

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

		ENGINE_ASSERT(vkCreateGraphicsPipelines(m_Context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) == VK_SUCCESS, "Pipeline creation failed");

		vkDestroyShaderModule(m_Context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Context->getDevice(), vertShaderModule, nullptr);
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
		ENGINE_ASSERT(vkCreateShaderModule(m_Context->getDevice(), &moduleInfo, nullptr, &shaderModule) == VK_SUCCESS, "Shader module creation failed")
			return shaderModule;
	}

	void Engine::initFramebuffers()
	{
		m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

		for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
		{
			VkImageView attachments[] =
			{
			  m_SwapchainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &attachments[0];
			framebufferInfo.width = m_SwapchainExtent.width;
			framebufferInfo.height = m_SwapchainExtent.height;
			framebufferInfo.layers = 1;

			ENGINE_ASSERT(vkCreateFramebuffer(m_Context->getDevice(), &framebufferInfo, nullptr, &m_SwapchainFramebuffers[i]) == VK_SUCCESS, "Framebuffer creation failed");
		}
	}

	void Engine::initRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapchainImageFormat;
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

		ENGINE_ASSERT(vkCreateRenderPass(m_Context->getDevice(), &renderPassInfo, nullptr, &m_RenderPass) == VK_SUCCESS, "Render pass creation failed");
	}

	void Engine::initCommandPool()
	{
		ENGINE_ASSERT(m_Context->getQueueIndices().graphicsFamily.has_value(), "Graphics queue family doesn't exist");

		QueueFamilyIndex graphicsFamilyIndex = m_Context->getQueueIndices().graphicsFamily.value();

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		commandPoolInfo.queueFamilyIndex = (uint32_t)graphicsFamilyIndex;

		ENGINE_ASSERT(vkCreateCommandPool(m_Context->getDevice(), &commandPoolInfo, nullptr, &m_CommandPool) == VK_SUCCESS, "Command pool creation failed");
	}

	void Engine::initCommandBuffer()
	{
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		ENGINE_ASSERT(vkAllocateCommandBuffers(m_Context->getDevice(), &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS, "Command buffer allocation failed")
	}

	void Engine::initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateBuffer(m_Context->getDevice(), &bufferInfo, nullptr, &buffer) == VK_SUCCESS, "Buffer creation failed");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_Context->getDevice(), buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		ENGINE_ASSERT(vkAllocateMemory(m_Context->getDevice(), &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Memory allocation failed");

		vkBindBufferMemory(m_Context->getDevice(), buffer, bufferMemory, 0);
	}

	void Engine::initVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Context->getDevice(), stagingBufferMemory);

		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

		copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(m_Context->getDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_Context->getDevice(), stagingBufferMemory, nullptr);
	}

	void Engine::initIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(uint16_t) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(m_Context->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(m_Context->getDevice(), stagingBufferMemory);

		initBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

		copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(m_Context->getDevice(), stagingBuffer, nullptr);
		vkFreeMemory(m_Context->getDevice(), stagingBufferMemory, nullptr);
	}

	void Engine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_Context->getDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_Context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_Context->getGraphicsQueue());
		vkFreeCommandBuffers(m_Context->getDevice(), m_CommandPool, 1, &commandBuffer);
	}
	uint32_t Engine::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_Context->getPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		ENGINE_ASSERT(false, "There is no suitable type of memory for buffer allocation");
		return 0;
	}
	void Engine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		ENGINE_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "Beginning of command buffer failed");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_SwapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_SwapchainExtent;

		VkClearValue clearColor = { {{0.850f, 0.796f, 0.937f, 1.0f}} };
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
		viewport.width = static_cast<float>(m_SwapchainExtent.width);
		viewport.height = static_cast<float>(m_SwapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_SwapchainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSets[currentFrame], 0, nullptr);

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

		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			ENGINE_ASSERT
			(
				vkCreateSemaphore(m_Context->getDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) == VK_SUCCESS &&
				vkCreateSemaphore(m_Context->getDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(m_Context->getDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) == VK_SUCCESS, "Sync objects creation failed"
			);
		}
	}

	//bool Engine::checkValidationLayerSupport()
	//{
	//	uint32_t layerCount;
	//	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	//	std::vector<VkLayerProperties> availableLayers(layerCount);
	//	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	//	for (const char* layerName : validationLayers) {
	//		bool layerFound = false;

	//		for (const auto& layerProperties : availableLayers) {
	//			if (strcmp(layerName, layerProperties.layerName) == 0) {
	//				layerFound = true;
	//				break;
	//			}
	//		}

	//		if (!layerFound)
	//			return false;
	//	}
	//	return true;
	//}

	//VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	//{
	//	switch (messageSeverity)
	//	{
	//	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
	//		//	ENGINE_TRACE(pCallbackData->pMessage);
	//		break;
	//	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
	//		ENGINE_INFO(pCallbackData->pMessage);
	//		break;
	//	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
	//		ENGINE_WARN(pCallbackData->pMessage);
	//		break;
	//	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
	//		ENGINE_FATAL(pCallbackData->pMessage);
	//		break;
	//	}
	//	return VK_FALSE;
	//}

	////void Engine::setupDebugMessanger()
	////{
	////	if (!enableValidationLayers) return;

	////	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	////	populateDebugMessengerCreateInfo(createInfo);

	////	ENGINE_ASSERT(createDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) == VK_SUCCESS, "Debug messanger creation failed");
	////}

	//VkResult Engine::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	//{
	//	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	//	if (func != nullptr)
	//	{
	//		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	//	}
	//	else {
	//		return VK_ERROR_EXTENSION_NOT_PRESENT;
	//	}
	//}

	//void Engine::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	//{
	//	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	//	if (func != nullptr)
	//	{
	//		func(instance, debugMessenger, pAllocator);
	//	}
	//}

	//void Engine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	//{
	//	createInfo = {};
	//	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	//	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	//	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	//	createInfo.pfnUserCallback = debugCallback;
	//	createInfo.pUserData = nullptr;
	//}

	//bool Engine::checkDeviceExtensionSupport(VkPhysicalDevice device)
	//{
	//	uint32_t extensionCount;
	//	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	//	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	//	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	//	std::unordered_set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	//	for (const auto& extension : availableExtensions)
	//	{
	//		requiredExtensions.erase(extension.extensionName);
	//	}

	//	return requiredExtensions.empty();
	//}
	//SwapChainSupportDetails Engine::querySwapChainSupport(VkPhysicalDevice device)
	//{
	//	SwapChainSupportDetails details;
	//	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vkSurface, &details.capabilities);

	//	uint32_t formatCount;
	//	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, nullptr);
	//	if (formatCount != 0)
	//	{
	//		details.formats.resize(formatCount);
	//		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vkSurface, &formatCount, details.formats.data());
	//	}

	//	uint32_t presentMode;
	//	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentMode, nullptr);
	//	if (presentMode != 0)
	//	{
	//		details.presentModes.resize(presentMode);
	//		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vkSurface, &presentMode, details.presentModes.data());
	//	}

	//	return details;
	//}
	//bool Engine::extensionsAvailability(const char** requiredExt, uint32_t extensNum)
	//{
	//	uint32_t extensionCount = 0;
	//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//	std::vector<VkExtensionProperties> avaibleExtensions(extensionCount);

	//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, avaibleExtensions.data());

	//	for (size_t i = 0; i < extensNum; i++)
	//	{
	//		bool exists = false;
	//		for (const auto& ext : avaibleExtensions)
	//		{
	//			if (strcmp(requiredExt[i], ext.extensionName))
	//				exists = true;
	//		}
	//		if (!exists) return false;
	//	}

	//	return true;
	//}
}