#include "Window.h"


namespace vkEngine
{

	Window::Window(uint32_t width, uint32_t height, const std::string& title) : m_InitialWidth(width), m_InitialHeight(height), m_Title(title)
	{
		initWindow();
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_glfwWindow);
		glfwTerminate();
	}

	void Window::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_glfwWindow = glfwCreateWindow(m_InitialWidth, m_InitialHeight, m_Title.c_str(), nullptr, nullptr);
	}

	void Window::disableCursor(bool isDisabled)
	{
		if (isDisabled)
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void Window::createSurface(VkInstance instance, VkSurfaceKHR& surface)
	{
		ENGINE_ASSERT(glfwCreateWindowSurface(instance, m_glfwWindow, nullptr, &surface) == VK_SUCCESS, "Window sufrace creation failed");
	}

	inline void Window::setWindowTitle(const char* title)
	{
		m_Title = title;
		glfwSetWindowTitle(m_glfwWindow, title);
	}

	std::pair<float, float> Window::getCursorPosition() const
	{
		double mouseX, mouseY;
		glfwGetCursorPos(m_glfwWindow, &mouseX, &mouseY);
		return { mouseX, mouseY };
	}

	std::pair<double, double> Window::getWindowSize() const
	{
		int width, height;
		glfwGetFramebufferSize(m_glfwWindow, &width, &height);
		return { width, height };
	}


}
