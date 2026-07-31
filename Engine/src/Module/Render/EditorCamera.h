#pragma once
#include "Module/Render/Camera.h"
#include "EngineCore/Core/Timestep.h"
#include "EngineCore/Events/Event.h"
#include "EngineCore/Events/MouseEvent.h"

#include "glm/glm.hpp"

namespace Himii {

    // 仅编辑器使用的自由视角相机（不进入 ECS）
    class EditorCamera : public Camera {
    public:
        EditorCamera() = default;
        EditorCamera(float fov,float aspectRatio,float nearClip,float farClip);
        
        void OnUpdate(Timestep ts, bool allowInput = true, bool is2D = false);
        void OnEvent(Event &event);

        void SetOrthographicProjection(bool enabled);
        bool IsOrthographicProjection() const
        {
            return m_UseOrthographicProjection;
        }

        inline float GetDistance() const
        {
            return m_Distance;
        }

        inline float SetDistance(float distance)
        {
            m_Distance = distance;
            return m_Distance;
        }

        inline void SetViewportSize(float width, float height)
        {
            m_ViewportWidth = width;
            m_ViewportHeight = height;
            m_AspectRatio = width / height;
            UpdateProjection();
        }

        const glm::mat4 &GetViewMatrix() const
        {
            return m_ViewMatrix;
        }

        glm::mat4 GetViewProjection() const
        {
            return m_Projection * m_ViewMatrix;
        }

        glm::vec3 GetUpDirection() const;
        glm::vec3 GetRightDirection() const;
        glm::vec3 GetForwardDirection() const;
        const glm::vec3 &GetPosition() const
        {
            return m_Position;
        }
        glm::quat GetOrientation() const;

        float GetPitch() const
        {
            return m_Pitch;
        }
        float GetYaw() const
        {
            return m_Yaw;
        }

        float GetNearClip() const { return m_NearClip; }
        float GetFarClip() const { return m_FarClip; }

    private:
        void UpdateProjection();
        void UpdateView();

        bool OnMouseScroll(MouseScrolledEvent &e);

        void MousePan(const glm::vec2 &delta, bool isTwoDimensional = false);
        void MouseRotate(const glm::vec2 &delta);
        void MouseZoom(float delta);

        glm::vec3 CalculatePosition() const;

        std::pair<float, float> PanSpeed() const;
        float RotationSpeed() const;
        float ZoomSpeed() const;

    private:
        float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

        glm::mat4 m_ViewMatrix;
        glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
        glm::vec3 m_FocalPoint = {0.0f, 0.0f, 0.0f};

        glm::vec2 m_InitialMousePosition = {0.0f, 0.0f};

        float m_Distance = 10.0f;
        float m_Pitch = 0.0f, m_Yaw = 0.0f;

        float m_ViewportWidth = 1280, m_ViewportHeight = 720;
        float m_MoveSpeed = 1.0f;

        bool m_UseOrthographicProjection = false;

        bool m_IsActive = false;
    };

} // namespace Himii
