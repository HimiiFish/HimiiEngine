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

    // 稳定 ID
    struct ID {
        UUID id;

        ID() = default;
        ID(const ID&) = default;
    };

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

    struct CameraComponent {
        SceneCamera camera;
        bool primary = true; // 主摄像机标记
        bool fixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent &) = default;
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


    struct MeshRenderer {
        Ref<VertexArray> vertexArray{};
        Ref<Shader>      shader{};
        Ref<Texture2D>   texture{};
    };

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
