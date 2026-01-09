#pragma once

#include "glm/glm.hpp"

namespace Himii
{

    enum class ProjectionType {
        Perspective = 0,
        Orthographic = 1
    };

    // 轻量父类：只定义接口与最少的矩阵/姿态缓存，行为由子类实现
    class Camera {
    public:
        // virtual ~Camera() = default;

        //// 生命周期
        // virtual void SetViewport(float width, float height) = 0;
        // virtual void SetClip(float nearZ, float farZ) = 0;
        // virtual void SetProjectionType(ProjectionType type) = 0;

        //// 姿态（默认无实现，由子类决定是否支持欧拉/LookAt/轨迹球等）
        // virtual void SetPosition(const glm::vec3& pos) = 0;
        // virtual void SetRotationEuler(const glm::vec3& eulerRadians) = 0;

        //// 查询
        // virtual glm::mat4 GetProjection() const = 0;
        // virtual glm::mat4 GetView() const = 0;
        // virtual glm::mat4 GetViewProjection() const = 0;

        Camera() = default;
        virtual ~Camera() = default;

        Camera(const glm::mat4 &projection) : m_Projection(projection)
        {
        }

        const glm::mat4 &GetProjection() const
        {
            return m_Projection;
        }

    protected:
        glm::mat4 m_Projection{1.0f};
    };
} // namespace Himii
