#include "pch.h"
#include "Texture2D.h"
#include "VulkanContext.h"
#include "CommandBufferHandler.h"
#include "Buffers/Buffer.h"

#include <stb_image.h>

namespace vkEngine
{
	Texture2D::Texture2D(const std::string& path, VkFormat format, bool enableMipmaps, bool enableAnisotropy)
		: m_Format(format), m_EnableMipmaps(enableMipmaps), m_EnableAnisotropy(enableAnisotropy)
	{
		loadTextureFromFile(path);
		createTextureSampler();
	}

	Texture2D::Texture2D(const std::string& path, bool enableMipmaps, bool enableAnisotropy)
		: m_EnableMipmaps(enableMipmaps), m_EnableAnisotropy(enableAnisotropy)
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

	void Texture2D::generateMipmaps()
	{
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
		samplerInfo.minLod = 0.0f;
		//TODO: like this m_EnableMipmaps ? static_cast<float>(m_Image->getMipLevels()) : 0.0f;
		samplerInfo.maxLod = 0.0f;

		ENGINE_ASSERT(vkCreateSampler(VulkanContext::getDevice(), &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS, "Failed to create texture sampler!");
	}

	void Texture2D::loadTextureFromFile(const std::string& path)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		ENGINE_ASSERT(pixels, (std::string("Failed to load image!") + " STB_IMAGE_FAILURE_REASON: " + stbi_failure_reason() + "! Path: " + path).c_str());

		Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.copyData(pixels, imageSize);

		stbi_image_free(pixels);

		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (m_EnableMipmaps)
			usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (m_Format)
		{
			m_Image = CreateScoped<Image2D>(
				VkExtent2D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) },
				m_Format,
				usageFlags,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
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

			m_Image = CreateScoped<Image2D>(
				VkExtent2D{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) },
				format,
				usageFlags,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
		}

		VkCommandBuffer cmdBuffer = VulkanContext::getCommandHandler()->beginSingleTimeCommands();

		m_Image->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		m_Image->copyBufferToImage(cmdBuffer, stagingBuffer.getBuffer(), texWidth, texHeight);

		if (m_EnableMipmaps)
			generateMipmaps();
		else
			m_Image->transitionImageLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		VulkanContext::getCommandHandler()->endSingleTimeCommands(cmdBuffer);
	}
}