#pragma once
// 基础组件定义（对标主流引擎：稳定ID、可读名称、Transform、SpriteRenderer、可选Camera）
// 注意：这是最小可用集，后续可扩展（Hierarchy、RigidBody等）。
#include "Hepch.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include "glm/gtc/quaternion.hpp" // glm::quat
#include <glm/gtx/quaternion.hpp>  // glm::toMat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale
#include <array>
#include <string>
#include <random>
#include "Himii/Core/Core.h"        // Ref<>
#include "Himii/Renderer/Texture.h" // Texture2D
#include "Himii/Renderer/VertexArray.h" // VertexArray for 3D meshes
#include "Himii/Renderer/Shader.h"      // Shader reference
#include "Himii/Core/UUID.h"
#include "Himii/Renderer/Camera.h"
#include "Himii/Scene/SceneCamera.h"
#include "Himii/Scene/ScriptableEntity.h"

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
        std::array<glm::vec2, 4> uvs{ // 默认全贴图 UV
            glm::vec2{0.0f, 0.0f}, glm::vec2{1.0f, 0.0f},
            glm::vec2{1.0f, 1.0f}, glm::vec2{0.0f, 1.0f}
        };
        float tiling = 1.0f;

        SpriteRenderer() = default;
        explicit SpriteRenderer(const glm::vec4 &tint)
            : color(tint) {}
        // 提供纹理 + 色调 + 平铺 的便捷构造（避免 emplace 的括号初始化失败）
        SpriteRenderer(const Ref<Texture2D> &tex, const glm::vec4 &tint, float tilingFactor)
            : color(tint), texture(tex), tiling(tilingFactor) {}
        // 提供纹理 + 自定义 UV + 平铺 + 色调 的构造
        SpriteRenderer(const Ref<Texture2D> &tex,
                       const std::array<glm::vec2, 4> &customUVs,
                       float tilingFactor,
                       const glm::vec4 &tint)
            : color(tint), texture(tex), uvs(customUVs), tiling(tilingFactor) {}
    };

    // 可选：作为主摄像机使用（暂不用于渲染主循环，留作扩展）
    struct CameraComponent {
        bool primary = false; // 是否为主摄像机
        ProjectionType projection = ProjectionType::Perspective;
        float fovYDeg = 45.0f; // 仅透视用
        float nearZ = 0.1f;
        float farZ = 100.0f;
        // 控制方式：是否使用 LookAt 目标（否则使用 Transform 的欧拉旋转）
        bool useLookAt = true;
        glm::vec3 lookAtTarget{0.0f, 0.0f, 0.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
    // 运行时场景相机对象
    Himii::SceneCamera camera{};
    };

    // 3D 网格渲染组件：用于 Renderer::Submit 路径
    struct MeshRenderer {
        Ref<VertexArray> vertexArray{};
        Ref<Shader>      shader{};
        Ref<Texture2D>   texture{}; // 可选：供 shader 取样
    };

    // 标记组件：用于区分天空盒渲染通道
    struct SkyboxTag {};

    // 脚本组件：原生 C++ 脚本挂载
    struct NativeScriptComponent {
        ScriptableEntity* Instance = nullptr;

        // 工厂函数与生命周期回调
        ScriptableEntity* (*InstantiateScript)() = nullptr;
        void (*DestroyScript)(NativeScriptComponent*) = nullptr;

        template<typename T>
        void Bind() {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) {
                delete nsc->Instance; nsc->Instance = nullptr; };
        }
    };

} // namespace Himii
