#include "pch.h"
#include "Texture2D.h"
#include "VulkanContext.h"
#include "CommandBufferHandler.h"
#include "Buffers/Buffer.h"

#include <stb_image.h>

namespace vkEngine
{



	Texture2D::Texture2D(const std::string& path, bool enableMipmaps, bool enableAnisotropy)
		: m_EnableMipmaps(enableMipmaps), m_EnableAnisotropy(enableAnisotropy)
	{
		loadTextureFromFile(path);
		createTextureSampler();
	}


	Texture2D::Texture2D(const std::string& path, VkFormat format, bool enableMipmaps, bool enableAnisotropy)
		: m_Format(format), m_EnableMipmaps(enableMipmaps), m_EnableAnisotropy(enableAnisotropy) 

	{
		loadTextureFromFile(path);
		createTextureSampler();
	}

	Texture2D::~Texture2D()
	{
		vkDestroySampler(VulkanContext::getDevice(), m_Sampler, nullptr);
	}

	VkDescriptorImageInfo Texture2D::getDescriptorImageInfo() const
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = getImageView();
		imageInfo.sampler = m_Sampler;
		return imageInfo;
	}

	void Texture2D::updateDescriptor(VkDescriptorSet descriptorSet, uint32_t binding)
	{
		VkDescriptorImageInfo imageInfo = getDescriptorImageInfo();

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(VulkanContext::getDevice(), 1, &descriptorWrite, 0, nullptr);
	}

	void Texture2D::generateMipmaps(VkCommandBuffer commandBuffer)
	{
		VkImage image = m_Image->getImage();
		Image2DConfig config = m_Image->getConfig();

		ENGINE_ASSERT(VulkanContext::getPhysicalDevice()->mipmapsSupport(config.format), "Mipmaps are not supported for this texture format!");

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = config.extent.width;
		int32_t mipHeight = config.extent.height;

		for (uint32_t i = 1; i < config.mipmapLevel; i++)
		{

			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = config.mipmapLevel - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	}

	void Texture2D::createTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = m_EnableMipmaps ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = m_EnableMipmaps ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		if (m_EnableAnisotropy)
		{
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
		}

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = m_EnableMipmaps ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = m_EnableMipmaps ? static_cast<float>(m_Image->getConfig().mipmapLevel) : 0.0f;

		ENGINE_ASSERT(vkCreateSampler(VulkanContext::getDevice(), &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS, "Failed to create texture sampler!");
	}

	void Texture2D::loadTextureFromFile(const std::string& path)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		ENGINE_ASSERT(pixels, (std::string("Failed to load image!") + " STB_IMAGE_FAILURE_REASON: " + stbi_failure_reason() + "! Path: " + path).c_str());

		uint32_t mipmapLevel = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.copyData(pixels, imageSize);

		stbi_image_free(pixels);

		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (m_EnableMipmaps)
			usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		Image2DConfig config{};

		if (m_Format)
		{
			config = {
				.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
				.format = m_Format,
				.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.usageFlags = usageFlags,
				.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipmapLevel = mipmapLevel
			};

			m_Image = CreateScoped<Image2D>(VulkanContext::getPhysicalDevice(), VulkanContext::getLogicalDevice(),
				config
			);
		}
		else
		{
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

			config = {
				.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)},
				.format = format,
				.memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.usageFlags = usageFlags,
				.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipmapLevel = mipmapLevel
			};

			m_Image = CreateScoped<Image2D>(VulkanContext::getPhysicalDevice(), VulkanContext::getLogicalDevice(),
				config
			);
		}

		VkCommandBuffer cmdBuffer = VulkanContext::getCommandHandler()->beginSingleTimeCommands();

		m_Image->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_Image->copyBufferToImage(cmdBuffer, stagingBuffer.getBuffer(), texWidth, texHeight);

		if (m_EnableMipmaps)
			generateMipmaps(cmdBuffer);
		else
			m_Image->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VulkanContext::getCommandHandler()->endSingleTimeCommands(cmdBuffer);
	}
}