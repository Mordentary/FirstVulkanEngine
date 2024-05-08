#pragma once
#pragma once

#include <vulkan/vulkan.h>

namespace vkEngine
{
    namespace VulkanUtils
    {
        VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);
        void endSingleTimeCommands(VkCommandPool commandPool, VkCommandBuffer commandBuffer);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    }
}
