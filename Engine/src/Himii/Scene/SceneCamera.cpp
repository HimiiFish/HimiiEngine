#include "Himii/Scene/SceneCamera.h"

namespace Himii
{
    SceneCamera::SceneCamera()
    {
        RecalculateProjection();
    }

    void SceneCamera::SetTarget(Entity target)
    {
        m_Target = target;
    }

    void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
    {
        m_OrthographicSize = size;
        m_OrthographicNear = nearClip;
        m_OrthographicFar = farClip;
        RecalculateProjection();
    }

    void SceneCamera::SetViewportSize(uint32_t width, uint32_t height)
    {
        m_AspectRatio = (height > 0) ? (float)width / (float)height : 1.0f;
        RecalculateProjection();
    }

    void SceneCamera::RecalculateProjection()
    {
        if (m_ProjectionType == ProjectionType::Perspective)
            m_Projection = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);

        else
        {
            float orthoLeft = -m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoRight = m_OrthographicSize * m_AspectRatio * 0.5f;
            float orthoBottom = -m_OrthographicSize * 0.5f;
            float orthoTop = m_OrthographicSize * 0.5f;
            m_Projection =
                    glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar);
        }
        
    }
} // namespace Himii
