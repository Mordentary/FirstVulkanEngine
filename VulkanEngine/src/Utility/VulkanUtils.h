#pragma once
#pragma once

#include <vulkan/vulkan.h>

namespace vkEngine
{
    namespace VulkanUtils
    {
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    }
}
