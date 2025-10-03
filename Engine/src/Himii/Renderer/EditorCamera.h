#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "Himii/Renderer/Camera.h"

namespace Himii {

    // 仅编辑器使用的自由视角相机（不进入 ECS）
    class EditorCamera : public Camera {
    public:
        EditorCamera() = default;
        ~EditorCamera() override = default;


        float& FovYDeg() { return m_FovYDeg; }
        float& Aspect() { return m_Aspect; }

        // 额外便捷：用 LookAt 直接设置视图
        void SetLookAt(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up);

    private:
        void RebuildProjection();
        void RecalculateView();

    private:
        ProjectionType m_Type{ProjectionType::Perspective};
        glm::mat4 m_Projection{1.0f};
        glm::mat4 m_View{1.0f};
        glm::vec3 m_Position{0.0f, 0.0f, 3.0f};
        glm::vec3 m_RotationEuler{0.0f};
        float m_FovYDeg{45.0f};
        float m_Aspect{16.0f/9.0f};
        float m_Near{0.1f};
        float m_Far{100.0f};
    };

} // namespace Himii
