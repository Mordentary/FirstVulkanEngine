#include "pch.h"

#include"Application.h"
#include"Core.h"

#include <exception>
#include <iostream>

int main()
{
	uint32_t width = 1000, height = 1000;
	vkEngine::Application app(DEBUG_BUILD_CONFIGURATION, width, height, "VulkanEngine");
	app.run();

	return EXIT_SUCCESS;
}