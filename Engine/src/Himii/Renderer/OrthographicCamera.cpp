#include "Hepch.h"
#include "Himii/Renderer/OrthographicCamera.h"

#include "glm/gtc/matrix_transform.hpp"

namespace Himii
{
    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top) 
        :m_ProjectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)),
          m_ViewMatrix(1.0f),
          m_ViewProjectionMatrix(m_ProjectionMatrix * m_ViewMatrix),
          m_Position(0.0f, 0.0f, 0.0f),
          m_Rotation(0.0f)
    {
        HIMII_PROFILE_FUNCTION();

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
    {
        HIMII_PROFILE_FUNCTION();

        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthographicCamera::RecalculateViewMatrix()
    {
        HIMII_PROFILE_FUNCTION();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position)*
                              glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        m_ViewMatrix = glm::inverse(transform);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
} // namespace Himii
