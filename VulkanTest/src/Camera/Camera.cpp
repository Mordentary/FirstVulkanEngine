#include "Camera.h"
#include "GLFW/glfw3.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>


namespace vkEngine
{

	Camera::Camera(const glm::vec3& position, const glm::vec3& cameraTarget, const Shared<Window> window, float FOV, const glm::vec2& nearFar)
		: m_NearFar(nearFar), m_Window(window), m_Position(position), m_FOVdeg(FOV)
		{
		if (position == cameraTarget)
		{
			ENGINE_ASSERT(false, "Camera target and position are equal!");
		}

		m_CameraSpaceAxisZ = -glm::normalize(position - cameraTarget);
		m_CameraSpaceAxisX = glm::normalize(glm::cross(m_CameraSpaceAxisZ, m_WorldSpaceAxisY));
		m_CameraSpaceAxisY = -glm::cross(m_CameraSpaceAxisZ, m_CameraSpaceAxisX);


		m_ViewMatrix = glm::lookAt(position, position + m_CameraSpaceAxisZ, m_CameraSpaceAxisY);
		auto [width, height] = window->getWindowSize();
		m_AspectRatio = static_cast<float>(width/height);
			
		m_ProjectionMatrix = glm::perspective(FOV, m_AspectRatio, nearFar.x, nearFar.y);
	}

	void Camera::Update(Timestep deltaTime)
	{
		UpdateCameraOrientation(deltaTime);
		UpdateCameraDisplacement(deltaTime);
		RecalculateViewProjection();
	}

	void Camera::UpdateCameraOrientation(Timestep dt)
	{
		static bool firstClick = true;

		if (glfwGetMouseButton(m_Window->getWindowGLFW(), GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
		{
			GLFWwindow* window = m_Window->getWindowGLFW();

			auto [width, height] = m_Window->getWindowSize();
			const float& sensitivity = m_CameraSensivity;

			m_Window->disableCursor(true);

			if (firstClick)
			{
				m_Window->setCursorPosition(width / 2, height / 2);
				firstClick = false;
			}

			auto [mouseX, mouseY] = m_Window->getCursorPosition();

			float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
			float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;


			glm::vec3 newOrientation = glm::rotate(m_CameraSpaceAxisZ, glm::radians(-rotX), glm::normalize(glm::cross(m_CameraSpaceAxisZ, m_WorldSpaceAxisY)));

			if (abs(glm::angle(newOrientation, m_WorldSpaceAxisY) - glm::radians(90.0f)) <= glm::radians(85.0f))
			{
				m_CameraSpaceAxisZ = newOrientation;
			}

			m_CameraSpaceAxisZ = glm::normalize(glm::rotate(m_CameraSpaceAxisZ, glm::radians(-rotY), m_WorldSpaceAxisY));

			m_CameraSpaceAxisX = glm::normalize(glm::cross(m_CameraSpaceAxisZ, m_WorldSpaceAxisY));
			m_CameraSpaceAxisY = -glm::cross(m_CameraSpaceAxisZ, m_CameraSpaceAxisX);

			m_CameraForward = m_CameraSpaceAxisZ;
			m_Window->setCursorPosition(width / 2, height / 2);
		}
		else if (glfwGetMouseButton(m_Window->getWindowGLFW(), GLFW_MOUSE_BUTTON_2) == GLFW_RELEASE)
		{
			m_Window->disableCursor(false);
			firstClick = true;
		}
	}

	void Camera::UpdateCameraDisplacement(Timestep ts)
	{
		auto window = m_Window->getWindowGLFW();
		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_W) == GLFW_PRESS)
			m_Position += ts * m_CameraTranslationSpeed * m_CameraSpaceAxisZ;

		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_S) == GLFW_PRESS)
			m_Position -= ts * m_CameraTranslationSpeed * m_CameraSpaceAxisZ;

		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_A) == GLFW_PRESS)
			m_Position -= glm::normalize(glm::cross(m_CameraSpaceAxisZ, m_CameraSpaceAxisY)) * (m_CameraTranslationSpeed * ts);

		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_D) == GLFW_PRESS)
			m_Position += glm::normalize(glm::cross(m_CameraSpaceAxisZ, m_CameraSpaceAxisY)) * (m_CameraTranslationSpeed * ts);

		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			m_Position += ts * m_CameraTranslationSpeed * m_WorldSpaceAxisY;
		}
		else if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		{
			m_Position -= ts * m_CameraTranslationSpeed * m_WorldSpaceAxisY;
		}

		static bool Initial = true;
		static float InitialSpeed;
		if (glfwGetKey(m_Window->getWindowGLFW(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		{
			if (Initial)
			{
				InitialSpeed = m_CameraTranslationSpeed;
				Initial = false;
			}
			m_CameraTranslationSpeed = (InitialSpeed * ts) + 5;
		}
		else
		{
			if (!Initial)
			{
				Initial = true;
				m_CameraTranslationSpeed = InitialSpeed;
			}
		}
	}

	void Camera::RecalculateViewProjection()
	{
		RecalculateProjection();
		RecalculateView();
		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	inline void Camera::RecalculateView()
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_CameraSpaceAxisZ, m_CameraSpaceAxisY);
		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	inline void Camera::RecalculateProjection()
	{
		auto [width, height] = m_Window->getWindowSize();
		m_AspectRatio = static_cast<float>(width / height);
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOVdeg), (float)m_AspectRatio, m_NearFar.x, m_NearFar.y);
		m_ProjectionViewMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}
