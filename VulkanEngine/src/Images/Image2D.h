#pragma once

#include "VulkanContext.h"

namespace vkEngine
{

	struct Image2DInfo
	{
		VkExtent2D extent{};
		VkFormat format{};
		VkImageUsageFlags usageFlags{};
		VkImageView imageView{};
		VkDeviceMemory imageMemory{};
	};

	class Image2D
	{
	public:
		// Constructors
		Image2D(VkExtent2D extent, VkFormat format, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags properties);
		// Destructor
		~Image2D();
		// Disable copy and assignment
		Image2D(const Image2D&) = delete;
		Image2D& operator=(const Image2D&) = delete;


		void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t width, uint32_t height);
			// Function to transition the image layout
		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

		// Function to copy data to the image (for example, texture data)
		void cleanup();

		// Getters
		VkImage getImage() const { return m_Image; }
		VkImageView getImageView() const { return m_ImageInfo.imageView; }

	private:
		VkDevice m_Device;
		VkImage m_Image;
		Image2DInfo m_ImageInfo;

	private:
		void createImage(VkMemoryPropertyFlags properties);
		void createImageView();
	};
}

