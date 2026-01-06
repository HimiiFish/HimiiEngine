#pragma once
#include "SceneCamera.h"
#include "Himii/Core/UUID.h"
#include "Himii/Renderer/Texture.h"
//#include "Himii/Renderer/Font.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>


namespace Himii
{
    class ScriptableEntity;

    // 绋冲畾 ID
    struct IDComponent {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent &) = default;
    };

    struct TagComponent {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string &name) : Tag(name)
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
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent &) = default;
    };

    struct SpriteRendererComponent {
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        Ref<Texture2D> Texture{};
        float TilingFactor = 1.0f;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent&) = default;
        SpriteRendererComponent(const glm::vec4 &color) : Color(color)
        {
        }
    };
    
    struct CircleRendererComponent {
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        float Radius = 0.5f;
        float Thickness = 1.0f;
        float Fade = 0.005f;

        CircleRendererComponent() = default;
        CircleRendererComponent(const CircleRendererComponent &) = default;
    };

    struct ScriptComponent {
        std::string ClassName;

        ScriptComponent() = default;
        ScriptComponent(const ScriptComponent &) = default;
    };

    class ScriptableEntity;

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

    struct Rigidbody2DComponent 
    {
        enum class BodyType {
            Static = 0,
            Dynamic,
            Kinematic
        };

        BodyType Type = BodyType::Static;
        bool FixedRotation = false;

        // Runtime
        void *RuntimeBody = nullptr;

        Rigidbody2DComponent() = default;
        Rigidbody2DComponent(const Rigidbody2DComponent &other) = default;
    };

    struct BoxCollider2DComponent
    {
        glm::vec2 Offset = {0.0f, 0.0f};
        glm::vec2 Size = {1.0f, 1.0f};

        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float RestitutionThreshold = 0.5f;

        // 运行时存储 FixtureId
        void *RuntimeFixture = nullptr;

        BoxCollider2DComponent() = default;
        BoxCollider2DComponent(const BoxCollider2DComponent &) = default;
    };

    struct CircleCollider2DComponent {
        glm::vec2 Offset = {0.0f, 0.0f};
        float Radius = 0.5f;

        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;
        float RestitutionThreshold = 0.5f;

        // 运行时存储 FixtureId
        void *RuntimeFixture = nullptr;

        CircleCollider2DComponent() = default;
        CircleCollider2DComponent(const CircleCollider2DComponent &) = default;
    };

    struct SpriteAnimationComponent {
        AssetHandle AnimationHandle = 0; // 引用 SpriteAnimation 资产

        float Timer = 0.0f;
        int CurrentFrame = 0;
        float FrameRate = 10.0f; // 默认 10 帧/秒
        bool Playing = true;

        SpriteAnimationComponent() = default;
        SpriteAnimationComponent(const SpriteAnimationComponent &) = default;
    };
}
