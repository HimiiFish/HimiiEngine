#include "Himii/Scene/SceneCamera.h"

namespace Himii {

void SceneCamera::SetPerspective(float fovYDeg, float aspect, float nearZ, float farZ) {
    m_Type = ProjectionType::Perspective;
    m_FovYDeg = fovYDeg; m_Aspect = aspect; m_Near = nearZ; m_Far = farZ;
    m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
}

void SceneCamera::SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
    m_Type = ProjectionType::Orthographic;
    m_Projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
}

void SceneCamera::SetViewport(float width, float height) {
    m_Aspect = (height > 0.0f) ? (width / height) : m_Aspect;
    if (m_Type == ProjectionType::Perspective)
        m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
}

void SceneCamera::SetClip(float nearZ, float farZ) {
    m_Near = nearZ; m_Far = farZ;
    if (m_Type == ProjectionType::Perspective)
        m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
}

void SceneCamera::SetProjectionType(ProjectionType type) {
    m_Type = type;
    if (m_Type == ProjectionType::Perspective)
        m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
}

void SceneCamera::SetPosition(const glm::vec3& pos) { m_Position = pos; RecalculateView(); }
void SceneCamera::SetRotationEuler(const glm::vec3& eulerRadians) { m_RotationEuler = eulerRadians; RecalculateView(); }

void SceneCamera::RecalculateView() {
    glm::mat4 rot = glm::toMat4(glm::quat(m_RotationEuler));
    glm::mat4 world = glm::translate(glm::mat4(1.0f), m_Position) * rot;
    m_View = glm::inverse(world);
}

} // namespace Himii
