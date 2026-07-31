#include "Module/Render/Renderer/EditorCamera.h"
#include "Hepch.h"

#include "EngineCore/Core/Input.h"
#include "EngineCore/Core/KeyCodes.h"
#include "EngineCore/Core/MouseCodes.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

namespace Himii
{
    EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip) :
        m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip),
        Camera(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
    {
        UpdateView();
    }

    void EditorCamera::SetOrthographicProjection(bool enabled)
    {
        if (m_UseOrthographicProjection == enabled)
            return;

        m_UseOrthographicProjection = enabled;

        if (enabled)
        {
            m_Pitch = 0.0f;
            m_Yaw = 0.0f;
            if (m_Distance < 1.0f)
                m_Distance = 10.0f;
        }

        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::UpdateProjection()
    {
        if (m_ViewportHeight < 1.0f)
            return;

        m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

        if (m_UseOrthographicProjection)
        {
            const float orthographicHalfHeight = m_Distance;
            const float orthographicHalfWidth = orthographicHalfHeight * m_AspectRatio;
            m_Projection = glm::ortho(-orthographicHalfWidth, orthographicHalfWidth, -orthographicHalfHeight,
                                      orthographicHalfHeight, m_NearClip, m_FarClip);
        }
        else
        {
            m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        }
    }

    void EditorCamera::UpdateView()
    {
        // m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
        m_Position = CalculatePosition();

        glm::quat orientation = GetOrientation();
        m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
        m_ViewMatrix = glm::inverse(m_ViewMatrix);
    }

    std::pair<float, float> EditorCamera::PanSpeed() const
    {
        float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
        float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

        float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
        float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

        return {xFactor, yFactor};
    }

    float EditorCamera::RotationSpeed() const
    {
        return 0.8f;
    }

    float EditorCamera::ZoomSpeed() const
    {
        float distance = m_Distance * 0.2f;
        distance = std::max(distance, 0.0f);
        float speed = distance * distance;
        speed = std::min(speed, 100.0f); // max speed = 100
        return speed;
    }

    void EditorCamera::OnUpdate(Timestep ts, bool allowInput, bool is2D)
    {
        const glm::vec2 &mouse{Input::GetMouseX(), Input::GetMouseY()};

        // 3D Mode Inputs
        bool isRight = Input::IsMouseButtonPressed(Mouse::ButtonRight);
        bool isMiddle = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);

        // 2D Mode Inputs
        // The user wants: "Only Scroll Zoom, Right Click Drag"
        bool is2DPan = isRight; 

        bool isAnyMouseButtonPressed = isRight || isMiddle;

        // Check Input Availability
        if (isAnyMouseButtonPressed)
        {
            if (!m_IsActive && allowInput)
            {
                m_IsActive = true;
                m_InitialMousePosition = mouse;
            }
        }
        else
        {
            m_IsActive = false;
        }

        if (m_IsActive)
        {
            glm::vec2 delta = is2D ? (mouse - m_InitialMousePosition)
                                   : (mouse - m_InitialMousePosition) * 0.01f;
            m_InitialMousePosition = mouse;
            
            if (is2D)
            {
                // 2D Mode Logic
                if (is2DPan)
                {
                    MousePan(delta, true);
                }
                
                if (m_Pitch != 0.0f || m_Yaw != 0.0f)
                {
                    m_Pitch = 0.0f;
                    m_Yaw = 0.0f;
                }
            }
            else
            {
                // 3D Mode Logic (Existing)
                if (isRight)
                {
                    MouseRotate(delta);
    
                    // WASD Movement
                    glm::vec3 direction(0.0f);
                    if (Input::IsKeyPressed(Key::W))
                        direction += GetForwardDirection();
                    if (Input::IsKeyPressed(Key::S))
                        direction -= GetForwardDirection();
                    if (Input::IsKeyPressed(Key::A))
                        direction -= GetRightDirection();
                    if (Input::IsKeyPressed(Key::D))
                        direction += GetRightDirection();
                    if (Input::IsKeyPressed(Key::Q))
                        direction -= GetUpDirection();
                    if (Input::IsKeyPressed(Key::E))
                        direction += GetUpDirection();
    
                    if (glm::length(direction) > 0.0f)
                        m_FocalPoint += direction * m_MoveSpeed * (float)ts;
                }
                else if (isMiddle)
                {
                    if (Input::IsKeyPressed(Key::LeftShift))
                        MouseZoom(delta.y);
                    else
                        MousePan(delta);
                }
            }
        }

        UpdateProjection();
        UpdateView();
    }

    void EditorCamera::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(EditorCamera::OnMouseScroll));
    }

    bool EditorCamera::OnMouseScroll(MouseScrolledEvent &e)
    {
        float delta = e.GetYOffset() * 0.1f;

        if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
        {
            m_MoveSpeed += e.GetYOffset() * 0.5f;
            if (m_MoveSpeed < 0.1f) m_MoveSpeed = 0.1f;
            if (m_MoveSpeed > 100.0f) m_MoveSpeed = 100.0f;
        }
        else
        {
            MouseZoom(delta);
            UpdateProjection();
            UpdateView();
        }
        
        return false;
    }

    void EditorCamera::MousePan(const glm::vec2 &delta, bool isTwoDimensional)
    {
        if (isTwoDimensional)
        {
            if (m_ViewportWidth <= 0.0f || m_ViewportHeight <= 0.0f)
                return;

            const float orthographicHalfHeight = m_Distance;
            const float orthographicHalfWidth = m_AspectRatio * m_Distance;
            const float worldUnitsPerPixelHorizontal = (2.0f * orthographicHalfWidth) / m_ViewportWidth;
            const float worldUnitsPerPixelVertical = (2.0f * orthographicHalfHeight) / m_ViewportHeight;

            m_FocalPoint += -GetRightDirection() * delta.x * worldUnitsPerPixelHorizontal;
            m_FocalPoint += GetUpDirection() * delta.y * worldUnitsPerPixelVertical;
            return;
        }

        auto [horizontalPanSpeed, verticalPanSpeed] = PanSpeed();
        m_FocalPoint += -GetRightDirection() * delta.x * horizontalPanSpeed * m_Distance;
        m_FocalPoint += GetUpDirection() * delta.y * verticalPanSpeed * m_Distance;
    }

    void EditorCamera::MouseRotate(const glm::vec2 &delta)
    {
        float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
        m_Yaw += yawSign * delta.x * RotationSpeed();
        m_Pitch += delta.y * RotationSpeed();
    }

    void EditorCamera::MouseZoom(float delta)
    {
        if (m_UseOrthographicProjection)
        {
            // 与 OrthographicCameraController 一致：每格滚轮约 0.25 世界单位（delta 已含 0.1 系数）
            m_Distance -= delta * 2.5f;
            if (m_Distance < 0.25f)
                m_Distance = 0.25f;
            return;
        }

        m_Distance -= delta * ZoomSpeed();
        if (m_Distance < 1.0f)
        {
            m_FocalPoint += GetForwardDirection();
            m_Distance = 1.0f;
        }
    }

    glm::vec3 EditorCamera::GetUpDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetRightDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 EditorCamera::GetForwardDirection() const
    {
        return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 EditorCamera::CalculatePosition() const
    {
        return m_FocalPoint - GetForwardDirection() * m_Distance;
    }

    glm::quat EditorCamera::GetOrientation() const
    {
        return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
    }
} // namespace Himii
