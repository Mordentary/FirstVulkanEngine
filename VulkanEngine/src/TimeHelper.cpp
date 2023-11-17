#include "pch.h"

#include "TimeHelper.h"
#include "GLFW/glfw3.h"

namespace vkEngine
{
	int32_t CurrentTime::GetCurrentTimeInSec_Int()
	{
		return static_cast<uint32_t>(glfwGetTime());
	}
	float CurrentTime::GetCurrentTimeInSec()
	{
		return static_cast<float>(glfwGetTime());
	}
}