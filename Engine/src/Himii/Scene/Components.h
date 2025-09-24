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
    struct TagComponent {
        std::string name;

        TagComponent() = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string &name) : name(name)
        {
        }
    };

    struct TransformComponent {
        glm::vec3 Position{0.0f};
        glm::vec3 Rotation{0.0f}; // Euler angles in radians
        glm::vec3 Scale{1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3 &position) : Position(position)
        {
        }
        glm::mat4 GetTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
        }
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
        Camera camera;
        bool primary = true; // 主摄像机标记

        CameraComponent() = default;
        CameraComponent(const CameraComponent &) = default;
        CameraComponent(const glm::mat4 &projection) : camera(projection)
        {
        }
    };

    // 3D 网格渲染组件：用于 Renderer::Submit 路径
    struct MeshRenderer {
        Ref<VertexArray> vertexArray{};
        Ref<Shader>      shader{};
        Ref<Texture2D>   texture{}; // 可选：供 shader 取样
    };

    // 脚本组件：原生 C++ 脚本挂载
    struct NativeScriptComponent {
        ScriptableEntity* Instance = nullptr;

        ScriptableEntity* (*InstantiateScript)();
        void (*DestroyScript)(NativeScriptComponent*);

        template<typename T>
        void Bind() {
            InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
            DestroyScript = [](NativeScriptComponent* nsc) {
                delete nsc->Instance; nsc->Instance = nullptr; };
        }
    };

} // namespace Himii
