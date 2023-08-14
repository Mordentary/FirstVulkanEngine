#pragma once
#include "../TimeHelper.h"
#include "../Window/Window.h"

#include <glm/glm.hpp>

namespace vkEngine
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::vec3& position, const glm::vec3& cameraTarget, const Shared<Window> window, float FOV = 90.0f, const glm::vec2& nearFar = { 0.1f,100.f });

		void Update(Timestep deltaTime);

		const inline glm::mat4& GetProjectionViewMatrix() const { return m_ProjectionViewMatrix; }
		const inline glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const inline glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }

		inline const glm::vec3& GetPosition() const { return m_Position; }

	private:
		void UpdateCameraOrientation(Timestep dt);
		void UpdateCameraDisplacement(Timestep deltaTime);

		void RecalculateView();
		void RecalculateViewProjection();
		void RecalculateProjection();
	private:
		Shared<Window> m_Window;
		glm::vec2 m_NearFar;

		float m_FOVdeg = 90.f;
		float m_CameraSensivity = 50.0f;
		float m_CameraTranslationSpeed = 0.2f;
		float m_AspectRatio = 1.6f;

		glm::vec3 m_Position{};
		glm::vec3& m_CameraForward = m_CameraSpaceAxisZ;

		glm::mat4 m_ViewMatrix{};
		glm::mat4 m_ProjectionMatrix{};
		glm::mat4 m_ProjectionViewMatrix{};

		const glm::vec3 m_WorldSpaceAxisY{ 0.0f,1.0f,0.0f };
		glm::vec3 m_CameraSpaceAxisX{ 1.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraSpaceAxisY{ 0.0f, 1.0f, 0.0f };
		glm::vec3 m_CameraSpaceAxisZ{ 0.0f, 0.0f, -1.0f };



	};
}
