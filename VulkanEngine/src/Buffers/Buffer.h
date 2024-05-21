#pragma once
#include "VulkanContext.h"

namespace vkEngine {
	class Buffer
	{
	public:
		Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		virtual ~Buffer();

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;
		
		void copyData(void* data, VkDeviceSize size) const;
		void copyBuffer(VkBuffer srcBuffer, VkDeviceSize size) const;

		VkBuffer getBuffer() const { return m_Buffer; }
		VkDeviceMemory getMemory() const { return m_Memory; }

	private:
		void initBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

	protected:
		VkBuffer m_Buffer;
		VkDeviceMemory m_Memory;
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 textureCoord;
		static VkVertexInputBindingDescription getBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, textureCoord);

			return attributeDescriptions;
		}
	};

	class VertexBuffer : public Buffer
	{
	public:
		VertexBuffer(const std::vector<Vertex>& vertices);
	};

	class IndexBuffer : public Buffer
	{
	public:
		IndexBuffer(const std::vector<uint32_t>& indices);
	};

}
