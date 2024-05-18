#include "pch.h"

#include "Window.h"
#include "VulkanContext.h"

namespace vkEngine
{

	void glfwErrorCallback(int error, const char* description)
	{
		ENGINE_ERROR("GLFW Error %s, : %s", error, description);
	}
	Window::Window(uint32_t width, uint32_t height, const std::string& title) : m_InitialExtent({ width, height }), m_Title(title)
	{
		ENGINE_ASSERT(width != 0 || height != 0, "Window height/width is 0");
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
		glfwSetErrorCallback(glfwErrorCallback);

		m_glfwWindow = glfwCreateWindow(m_InitialExtent.first, m_InitialExtent.second, m_Title.c_str(), nullptr, nullptr);
	}

	void Window::disableCursor(bool isDisabled)
	{
		if (isDisabled)
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void Window::createSurface(const Shared<Instance>& inst)
	{
		ENGINE_ASSERT(glfwCreateWindowSurface(inst->instance(), m_glfwWindow, nullptr, &m_Surface) == VK_SUCCESS, "Window sufrace creation failed");
	}

	void Window::destroySurface(const Shared<Instance>& inst)
	{
		ENGINE_ASSERT(m_Surface != VK_NULL_HANDLE, "Surface has not been created before attempting to destroy it");
		vkDestroySurfaceKHR(inst->instance(), m_Surface, nullptr);
		m_Surface = VK_NULL_HANDLE;
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
	bool Window::isMinimized() const
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_glfwWindow, &width, &height);
		return (width == 0 || height == 0);
	}

	std::pair<int, int> Window::getWindowSize() const {
		if (m_glfwWindow == nullptr) {
			// Return a default size or handle the error accordingly
			return { 0, 0 };
		}
		int width = 0, height = 0;
		glfwGetFramebufferSize(m_glfwWindow, &width, &height);
		return { width, height };
	}
}