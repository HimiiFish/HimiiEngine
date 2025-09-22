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

    struct TransformComponent {
        glm::mat4 Transform{1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::mat4 &transform) : Transform(transform)
        {
        }
        operator const glm::mat4&() { return Transform; }
        operator const glm::mat4&() const { return Transform; }
    };

    struct SpriteRendererComponent {
        glm::vec4 color{1.0f};
        Ref<Texture2D> texture{};
        float tilingFactor = 1.0f;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4 &color) : color(color)
        {
        }
    };

    // 可选：作为主摄像机使用（暂不用于渲染主循环，留作扩展）
    struct CameraComponent {
        bool primary = false; // 是否为主摄像机
        ProjectionType projection = ProjectionType::Perspective;
        float fovYDeg = 45.0f; // 仅透视用
        float nearZ = 0.1f;
        float farZ = 100.0f;
    // 正交相机的可视高度（用于缩放/zoom），保持垂直尺寸不变，水平按纵横比自适应
    float orthoSize = 10.0f;
    // 控制方式：是否使用 LookAt 目标（否则使用 Transform 的欧拉旋转）。默认关闭以便旋转独立。
    bool useLookAt = false;
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
