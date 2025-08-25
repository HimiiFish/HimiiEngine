#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Himii/Renderer/Camera.h"

namespace Himii {

    class SceneCamera : public Camera {
    public:
        SceneCamera() = default;
        ~SceneCamera() override = default;

        // Back-compat helpers (used by existing code)
        void SetPerspective(float fovYDeg, float aspect, float nearZ, float farZ);
        void SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ);
    // Convenience: set orthographic by size (vertical view height) and aspect
    void SetOrthographicBySize(float orthoSize, float aspect=(16.0f/9.0f), float nearZ=0.1f, float farZ=100.0f);

    // Parameter setters that rebuild projection using stored aspect/clip
    void SetFovYDeg(float fovYDeg);
    void SetOrthoSize(float orthoSize);

        // Camera interface
        void SetViewport(float width, float height) override;
        void SetClip(float nearZ, float farZ) override;
        void SetProjectionType(ProjectionType type) override;

        void SetPosition(const glm::vec3& pos) override;
        void SetRotationEuler(const glm::vec3& eulerRadians) override;

        glm::mat4 GetProjection() const override { return m_Projection; }
        glm::mat4 GetView() const override { return m_View; }
        glm::mat4 GetViewProjection() const override { return m_Projection * m_View; }

    private:
        void RecalculateView();

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
    float m_OrthoSize{10.0f};
    };

} // namespace Himii
