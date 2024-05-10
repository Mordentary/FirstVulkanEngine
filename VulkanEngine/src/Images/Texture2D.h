#pragma once

#include "Image2D.h"

namespace vkEngine
{


	class Texture2D
	{
	public:
		Texture2D(const std::string& path, VkFormat format, bool enableMipmaps = false, bool enableAnisotropy = true);
		Texture2D::Texture2D(const std::string& path, bool enableMipmaps = false, bool enableAnisotropy = true);
		~Texture2D();

		Texture2D(const Texture2D&) = delete;
		Texture2D& operator=(const Texture2D&) = delete;

		VkImageView getImageView() const { return m_Image->getImageView(); }
		VkSampler getSampler() const { return m_Sampler; }
		VkDescriptorImageInfo getDescriptorImageInfo() const;
		VkExtent2D getExtent() const { return m_Image->getExtent(); }

		void updateDescriptor(VkDescriptorSet descriptorSet, uint32_t binding);


	private:
		void generateMipmaps();
		void createTextureSampler();
		void loadTextureFromFile(const std::string& path);

	private:
		Shared<Image2D> m_Image = nullptr ;
		VkSampler m_Sampler = nullptr;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		bool m_EnableMipmaps = false;
		bool m_EnableAnisotropy = false;
	};
}
