#include "pch.h"

#include "Image2D.h"
#include "Logger/Logger.h"
#include "Utility/VulkanUtils.h"
#include "QueueHandler.h"

namespace vkEngine
{
	Image2D::Image2D(const Shared<PhysicalDevice>& phsDevice, const Shared<LogicalDevice>& device, const Image2DConfig& config)
		: m_Config(config), m_Device(device), m_PhysDevice(phsDevice)
	{
		if (config.usageFlags == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			return;

		createImage();
		createImageView();
	}

	Image2D::~Image2D()
	{
		cleanup();
	}

	void Image2D::cleanup() {
		VkDevice device = m_Device->logicalDevice();
		vkFreeMemory(device, m_Memory, nullptr);
		vkDestroyImageView(device, m_ImageView, nullptr);
		vkDestroyImage(device, m_Image, nullptr);
	}

	void Image2D::createImage() {
		VkDevice device = m_Device->logicalDevice();
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Config.extent.width;
		imageInfo.extent.height = m_Config.extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_Config.format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = m_Config.usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateImage(device, &imageInfo, nullptr, &m_Image) == VK_SUCCESS, "Failed to create image");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, m_Image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_PhysDevice->findMemoryType(memRequirements.memoryTypeBits, m_Config.memoryProperties);

		ENGINE_ASSERT(vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory) == VK_SUCCESS, "Failed to allocate memory for image");

		vkBindImageMemory(device, m_Image, m_Memory, 0);
	}

	void Image2D::createImageView() {
		VkDevice device = m_Device->logicalDevice();
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Config.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		ENGINE_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) == VK_SUCCESS, "Failed to create image view");
	}

	void Image2D::resize(uint32_t width, uint32_t height)
	{
		cleanup();

		m_Config.extent.width = width;
		m_Config.extent.height = height;

		createImage();
		createImageView();
	}

	void Image2D::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t width, uint32_t height)
	{
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			m_Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
	}

	void Image2D::transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			ENGINE_ASSERT(false, "Layout transition is not supported");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	void DepthImage::createImage()
	{
		VkDevice device = m_Device->logicalDevice();

		// Create depth image with the specified format, usage flags, and memory properties
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = m_Config.extent.width;
		imageInfo.extent.height = m_Config.extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = m_Config.format;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = m_Config.usageFlags;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ENGINE_ASSERT(vkCreateImage(device, &imageInfo, nullptr, &m_Image) == VK_SUCCESS,
			"Failed to create depth image!");

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, m_Image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_PhysDevice->findMemoryType(memRequirements.memoryTypeBits, m_Config.memoryProperties);



		ENGINE_ASSERT(vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory) == VK_SUCCESS,
			"Failed to allocate depth image memory!");

		vkBindImageMemory(device, m_Image, m_Memory, 0);
	}

	void DepthImage::createImageView()
	{
		VkDevice device = m_Device->logicalDevice();

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Config.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (m_PhysDevice->hasStencilComponent(m_Config.format))
		{
			viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		ENGINE_ASSERT(vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) == VK_SUCCESS,
			"Failed to create depth image view!");
	}

	DepthImage::DepthImage(const Shared<PhysicalDevice>& phsDevice, const Shared<LogicalDevice>& device, const Image2DConfig& config) : Image2D(phsDevice,device, config)
	{
		createImage();
		createImageView();
	};

	void DepthImage::transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (VulkanContext::getPhysicalDevice()->hasStencilComponent(m_Config.format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else {
			ENGINE_ASSERT(false, "Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
}