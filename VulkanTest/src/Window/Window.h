#pragma once

#include <string>
#include <vector>
#include <utility>
#include "Logger/Logger.h"
#include "Swapchain.h"

namespace vkEngine
{

	class Application;
	class Window
	{
		friend Application;
	public:

		Window() = default;
		Window(uint32_t width, uint32_t height, const std::string& title);
		~Window();

		inline GLFWwindow* getWindowGLFW() { return m_glfwWindow; }
		inline bool shouldClose() { return glfwWindowShouldClose(m_glfwWindow); };
		inline void setCursorPosition(float xPos, float yPos) { glfwSetCursorPos(m_glfwWindow, xPos, yPos); };
		
		std::pair<float, float> getCursorPosition();
		std::pair<double, double> getWindowSize();
		void disableCursor(bool IsDisabled);
		void pollEvents() { glfwPollEvents(); }
	private:
		void createSurface(VkInstance instance, VkSurfaceKHR& surface);
		void initWindow();
	private:
		GLFWwindow* m_glfwWindow{ nullptr };
		int m_Width = 1000, m_Height = 1000;
		std::string m_Title{};

	};
}
