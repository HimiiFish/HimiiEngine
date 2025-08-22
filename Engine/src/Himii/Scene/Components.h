#pragma once
// 基础组件定义（对标主流引擎：稳定ID、可读名称、Transform、SpriteRenderer、可选Camera）
// 注意：这是最小可用集，后续可扩展（Hierarchy、RigidBody等）。
#include "Hepch.h"
#include <glm/glm.hpp>
#include "glm/gtc/quaternion.hpp" // glm::quat
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>  // glm::toMat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale
#include <array>
#include <string>
#include <random>
#include "Himii/Core/Core.h"        // Ref<>
#include "Himii/Renderer/Texture.h" // Texture2D
#include "Himii/Core/UUID.h"

namespace Himii
{

    // 稳定 ID（用于引用、序列化）
    struct ID {
        UUID id;

        ID() = default;
        ID(const ID&) = default;
    };

    // 可读名称（在编辑器层显示/重命名）
    struct Tag {
        std::string name;
    };

    struct Transform {
        glm::vec3 Position = {0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        Transform() = default;
        Transform(const Transform&) = default;
        Transform(const glm::vec3 &translation) : Position(translation)
        {
        }

        glm::mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

            return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct SpriteRenderer {
        glm::vec4 color{1.0f};
        Ref<Texture2D> texture{};       // 可为空，仅颜色
        std::array<glm::vec2, 4> uvs{}; // 可选：自定义 UV（用于图集）
        float tiling = 1.0f;
    };

    // 可选：作为主摄像机使用（暂不用于渲染主循环，留作扩展）
    struct CameraComponent {
        bool primary = false; // 是否为主摄像机
        // 仅预留参数，实际投影由引擎相机类驱动
        float orthoSize = 10.0f;
        float nearClip = -1.0f;
        float farClip = 1.0f;
    };

} // namespace Himii
