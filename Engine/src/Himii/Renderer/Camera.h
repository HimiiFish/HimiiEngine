#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace Himii {

    enum class ProjectionType { Perspective = 0, Orthographic = 1 };

    class Camera {
    public:
        Camera() = default;

        // Perspective params are in degrees for FOV
        void SetPerspective(float fovYDeg, float aspect, float nearZ, float farZ) {
            m_Type = ProjectionType::Perspective;
            m_FovYDeg = fovYDeg; m_Aspect = aspect; m_Near = nearZ; m_Far = farZ;
            m_Projection = glm::perspective(glm::radians(m_FovYDeg), m_Aspect, m_Near, m_Far);
        }

        void SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
            m_Type = ProjectionType::Orthographic;
            m_Projection = glm::ortho(left, right, bottom, top, nearZ, farZ);
        }

        void SetPosition(const glm::vec3& pos) { m_Position = pos; RecalculateView(); }
        void SetRotationEuler(const glm::vec3& eulerRadians) { m_RotationEuler = eulerRadians; RecalculateView(); }

        const glm::vec3& GetPosition() const { return m_Position; }
        const glm::vec3& GetRotationEuler() const { return m_RotationEuler; }

        float GetFovYDeg() const { return m_FovYDeg; }
        float GetAspect() const { return m_Aspect; }
        float GetNear() const { return m_Near; }
        float GetFar() const { return m_Far; }

        ProjectionType GetProjectionType() const { return m_Type; }

        const glm::mat4& GetProjection() const { return m_Projection; }
        const glm::mat4& GetView() const { return m_View; }
        glm::mat4 GetViewProjection() const { return m_Projection * m_View; }

    private:
        void RecalculateView() {
            // Build world matrix from position and rotation (ignore scale for camera)
            glm::mat4 rot = glm::toMat4(glm::quat(m_RotationEuler));
            glm::mat4 world = glm::translate(glm::mat4(1.0f), m_Position) * rot;
            m_View = glm::inverse(world);
        }

    private:
        ProjectionType m_Type{ProjectionType::Perspective};
        glm::mat4 m_Projection{1.0f};
        glm::mat4 m_View{1.0f};
        glm::vec3 m_Position{0.0f, 0.0f, 3.0f};
        glm::vec3 m_RotationEuler{0.0f}; // radians
        float m_FovYDeg{45.0f};
        float m_Aspect{16.0f/9.0f};
        float m_Near{0.1f};
        float m_Far{100.0f};
    };
}
