#include "Himii/Renderer/EditorCamera.h"

namespace Himii {

void EditorCamera::SetViewport(float width, float height) {
    m_Aspect = (height > 0.0f) ? (width / height) : m_Aspect;
    RebuildProjection();
}

void EditorCamera::SetClip(float nearZ, float farZ) {
    m_Near = nearZ; m_Far = farZ; RebuildProjection();
}

void EditorCamera::SetProjectionType(ProjectionType type) {
    m_Type = type; RebuildProjection();
}

void EditorCamera::SetPosition(const glm::vec3& pos) { m_Position = pos; RecalculateView(); }
void EditorCamera::SetRotationEuler(const glm::vec3& eulerRadians) { m_RotationEuler = eulerRadians; RecalculateView(); }

void EditorCamera::SetLookAt(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
    m_Position = pos;
    m_View = glm::lookAt(pos, target, up);
}

void EditorCamera::RebuildProjection() {
    if (m_Type == ProjectionType::Perspective)
        m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
    else {
        float orthoH = 10.0f;
        float orthoW = orthoH * m_Aspect;
        m_Projection = glm::ortho(-orthoW*0.5f, orthoW*0.5f, -orthoH*0.5f, orthoH*0.5f, m_Near, m_Far);
    }
}

void EditorCamera::RecalculateView() {
    glm::mat4 rot = glm::toMat4(glm::quat(m_RotationEuler));
    glm::mat4 world = glm::translate(glm::mat4(1.0f), m_Position) * rot;
    m_View = glm::inverse(world);
}

} // namespace Himii
