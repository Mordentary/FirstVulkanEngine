#include "pch.h"
#include "VulkanUtils.h"
#include "QueueHandler.h"


namespace vkEngine
{


	uint32_t VulkanUtils::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties = VulkanContext::getPhysicalDevice()->getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		ENGINE_ASSERT(false, "There is no suitable type of memory for buffer allocation");
		return 0;
	}
}
