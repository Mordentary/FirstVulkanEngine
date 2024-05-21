#pragma once

#include "VulkanContext.h"

namespace vkEngine
{
	class LogicalDevice;
	class PhysicalDevice;

	struct Image2DConfig
	{
		VkExtent2D extent;
		VkFormat format;
		VkMemoryPropertyFlags memoryProperties;
		VkImageUsageFlags usageFlags;
		VkImageAspectFlags aspectFlags;
	};

	class Image2D
	{
	public:
		Image2D(const Shared<PhysicalDevice>& phsDevice, const Shared<LogicalDevice>& device, const Image2DConfig& config);
		virtual ~Image2D();

		// Disable copy and assignment
		Image2D(const Image2D&) = delete;
		Image2D& operator=(const Image2D&) = delete;

		virtual void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

		void resize(uint32_t width, uint32_t height);
		//void copyDataToImage(const void* data, VkDeviceSize size);
		void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, uint32_t width, uint32_t height);

		// Getters
		VkImage getImage() const { return m_Image; }
		VkImageView getImageView() const { return m_ImageView; }
		VkExtent2D getExtent() const { return m_Config.extent; }
		VkFormat getFormat() const { return m_Config.format; }

	private:
		void cleanup();
	protected:
		Image2D() = default;
		virtual void createImage();
		virtual void createImageView();

	protected:
		const Shared<LogicalDevice> m_Device = nullptr;
		const Shared<PhysicalDevice> m_PhysDevice = nullptr;
	protected:
		Image2DConfig m_Config;
		VkImage m_Image;
		VkImageView m_ImageView;
		VkDeviceMemory m_Memory;
	};

	class DepthImage : public Image2D
	{
	public:
		DepthImage(const Shared<PhysicalDevice>& phsDevice, const Shared<LogicalDevice>& device, const Image2DConfig& config);

		void transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout) override;
		// Disable copy and assignment
		DepthImage(const DepthImage&) = delete;
		DepthImage& operator=(const DepthImage&) = delete;

	protected:
		void createImage() override;
		void createImageView() override;
	};
}
