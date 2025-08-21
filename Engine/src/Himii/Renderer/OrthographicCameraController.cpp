#include "Hepch.h"
#include "Himii/Renderer/OrthographicCameraController.h"
#include "Himii/Core/Input.h"
#include "Himii/Core/KeyCodes.h"

namespace Himii
{
    OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation) :
        m_AspectRatio(aspectRatio),
        m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel),
        m_Rotation(rotation)
    {
    }
    void OrthographicCameraController::OnUpdate(Timestep ts)
    {
        HIMII_PROFILE_FUNCTION();

    // WASD 移动仅用于透视相机。正交相机不使用 WASD，这里不做平移输入处理。

        if (m_Rotation)
        {
            if (Input::IsKeyPressed(Key::Q))
                m_CameraRotation += m_CameraRotationSpeed * ts;
            if (Input::IsKeyPressed(Key::E))
                m_CameraRotation -= m_CameraRotationSpeed * ts;

            m_Camera.SetRotation(m_CameraRotation);
        }
        m_Camera.SetPosition(m_CameraPosition);
        m_CameraTranslationSpeed = m_ZoomLevel;
    }
    void OrthographicCameraController::OnEvent(Event &e)
    {
        HIMII_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
    dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(OrthographicCameraController::OnMouseMoved));
    dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(OrthographicCameraController::OnMouseButtonPressed));
    dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(OrthographicCameraController::OnMouseButtonReleased));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OrthographicCameraController::OnWindowResize));
    }
    void OrthographicCameraController::OnResize(float width, float height)
    {
        m_AspectRatio=width/height;
    m_ViewportWidth = width;
    m_ViewportHeight = height;
        m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel,m_ZoomLevel);
    }
    bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent &e)
    {
        HIMII_PROFILE_FUNCTION();

        m_ZoomLevel -= e.GetYOffset() * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.25f);
        m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
        return false;
    }
    bool OrthographicCameraController::OnMouseMoved(MouseMovedEvent &e)
    {
        if (!m_MiddleDragging)
            return false;

        // 把屏幕像素位移换算到世界坐标位移
        glm::vec2 mousePos{e.GetX(), e.GetY()};
        glm::vec2 delta = mousePos - m_LastMousePos;
        m_LastMousePos = mousePos;

        if (m_ViewportWidth <= 0.0f || m_ViewportHeight <= 0.0f)
            return false;

        float worldHalfH = m_ZoomLevel;                 // 正交上下为 [-zoom, +zoom]
        float worldHalfW = m_AspectRatio * m_ZoomLevel; // 左右范围 [-aspect*zoom, +aspect*zoom]
        float worldPerPixelX = (2.0f * worldHalfW) / m_ViewportWidth;
        float worldPerPixelY = (2.0f * worldHalfH) / m_ViewportHeight;

        // 屏幕坐标 y 向下为正，世界 y 向上为正，需要取反
        m_CameraPosition.x -= delta.x * worldPerPixelX;
        m_CameraPosition.y += delta.y * worldPerPixelY * 1.0f;
        m_Camera.SetPosition(m_CameraPosition);
        return false;
    }
    bool OrthographicCameraController::OnMouseButtonPressed(MouseButtonPressedEvent &e)
    {
        if (e.GetMouseButton() == Mouse::ButtonMiddle)
        {
            m_MiddleDragging = true;
            // 记录初始位置
            m_LastMousePos = { Input::GetMouseX(), Input::GetMouseY() };
        }
        return false;
    }
    bool OrthographicCameraController::OnMouseButtonReleased(MouseButtonReleasedEvent &e)
    {
        if (e.GetMouseButton() == Mouse::ButtonMiddle)
        {
            m_MiddleDragging = false;
        }
        return false;
    }
    bool OrthographicCameraController::OnWindowResize(WindowResizeEvent &e)
    {
        HIMII_PROFILE_FUNCTION();

        OnResize((float)e.GetWidth(), (float)e.GetHeight());
        return false;
    }
} // namespace Himii
