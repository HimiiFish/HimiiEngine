#pragma once
#include "SceneCamera.h"
#include "EngineCore/Core/UUID.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Script/ScriptEngine.h"
#include "Module/Render/Renderer/Font.h"
#include "Module/Audio/SoundAsset.h"
#include "Module/Audio/AudioEngine.h"
#include "Resource/Sprite.h"
#include "Module/Animation/SpriteAnimation.h"

#include <array>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/quaternion.hpp>
#include <vector>


namespace Himii
{
    class ScriptableEntity;

    // Entity ID
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

    struct RelationshipComponent {
        UUID Parent = 0;
        uint32_t SiblingIndex = 0;

        RelationshipComponent() = default;
        RelationshipComponent(const RelationshipComponent &) = default;
    };

    struct TransformComponent {
        glm::vec3 Position{0.0f};
        glm::vec3 Rotation{0.0f}; // Euler angles in radians, local space
        glm::vec3 Scale{1.0f};

        mutable bool WorldTransformDirty = true;
        mutable glm::mat4 CachedWorldTransform{1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3 &position) : Position(position)
        {
        }

        glm::mat4 GetLocalTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
            return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
        }

        glm::mat4 GetTransform() const
        {
            return GetLocalTransform();
        }
    };

    struct CameraComponent {
        SceneCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        CameraComponent() = default;
        CameraComponent(const CameraComponent &) = default;
    };

    class AssetManager;

    struct SpriteRendererComponent {
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        AssetHandle SpriteAssetHandle = 0;
        float TilingFactor = 1.0f;
        bool FlipHorizontal = false;
        int SortingLayer = 0;
        int SortingOrder = 0;

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

    struct MeshComponent {
        enum class MeshSource
        {
            Builtin = 0,
            Asset = 1
        };

        enum class MeshType
        {
            Cube = 0,
            Plane = 1,
            Sphere = 2,
            Capsule = 3
        };

        MeshSource Source = MeshSource::Builtin;
        MeshType Type = MeshType::Cube;
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        AssetHandle MeshAssetHandle = 0;
        std::vector<AssetHandle> MaterialAssetHandles;

        MeshComponent() = default;
        MeshComponent(const MeshComponent&) = default;
    };

    struct ScriptComponent {
        std::string ClassName;

        ScriptFieldMap Fields;

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

        bool IsTrigger = false;
        int Layer = 0;

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

        bool IsTrigger = false;
        int Layer = 0;

        // 运行时存储 FixtureId
        void *RuntimeFixture = nullptr;

        CircleCollider2DComponent() = default;
        CircleCollider2DComponent(const CircleCollider2DComponent &) = default;
    };

    struct SpriteAnimationComponent {
        AssetHandle AnimationHandle = 0;
        std::string CurrentAnimationName = SpriteAnimationDefaultClipName;

        float Timer = 0.0f;
        int CurrentFrame = 0;
        int PlaybackDirection = 1;
        float FrameRate = 10.0f;
        bool Playing = true;
        bool PreviewInScene = false;

        SpriteAnimationComponent() = default;
        SpriteAnimationComponent(const SpriteAnimationComponent &) = default;
    };

    struct TilemapComponent {
        // 引用外部 TileMapData 资源（包含 TileSet 引用 + 地图数据）
        AssetHandle TileMapHandle = 0;

        TilemapComponent() = default;
        TilemapComponent(const TilemapComponent&) = default;
    };

    struct TilemapCollider2DComponent
    {
        bool Enabled = true;
        bool MergeAdjacentCells = false;

        TilemapCollider2DComponent() = default;
        TilemapCollider2DComponent(const TilemapCollider2DComponent&) = default;
    };

    struct ParticleEmitterComponent
    {
        AssetHandle EmitterHandle = 0;

        // 运行时发射累计时间，不序列化
        float EmissionAccumulator = 0.0f;

        ParticleEmitterComponent() = default;
        ParticleEmitterComponent(const ParticleEmitterComponent&) = default;
    };

#pragma region UIComponent
    enum class CanvasScaleMode
    {
        ConstantPixelSize = 0,
        ScaleWithScreenSize = 1
    };

    struct CanvasComponent {
        CanvasScaleMode ScaleMode = CanvasScaleMode::ScaleWithScreenSize;
        glm::vec2 ReferenceResolution{1920.0f, 1080.0f};
        /// 0 = 按宽度适配，1 = 按高度适配，0.5 = 折中。
        float MatchWidthOrHeight = 0.5f;

        CanvasComponent() = default;
        CanvasComponent(const CanvasComponent &) = default;
    };

    struct RectTransformComponent {
        glm::vec2 AnchorMinimum{0.5f, 0.5f};
        glm::vec2 AnchorMaximum{0.5f, 0.5f};
        glm::vec2 Pivot{0.5f, 0.5f};
        glm::vec2 AnchoredPosition{0.0f};
        glm::vec2 SizeDelta{100.0f, 100.0f};
        float RotationRadians = 0.0f;

        mutable bool WorldTransformDirty = true;
        mutable glm::mat4 CachedWorldTransform{1.0f};
        mutable glm::vec2 ResolvedSize{100.0f, 100.0f};

        RectTransformComponent() = default;
        RectTransformComponent(const RectTransformComponent &) = default;

        glm::mat4 GetLocalTransform() const
        {
            return glm::translate(
                           glm::mat4(1.0f),
                           glm::vec3(AnchoredPosition, 0.0f))
                   * glm::rotate(
                           glm::mat4(1.0f), RotationRadians,
                           glm::vec3(0.0f, 0.0f, 1.0f));
        }

        glm::mat4 GetTransform() const
        {
            return GetLocalTransform();
        }
    };

    struct UIImageComponent {
        Ref<Texture2D> Texture;
        AssetHandle TextureHandle = 0;
        glm::vec4 Color{1.0f, 1.0f, 1.0f, 1.0f};

        UIImageComponent() = default;
        UIImageComponent(const UIImageComponent &) = default;
    };

    struct UITextComponent {
        std::string TextString = "Text";
        Ref<Font> FontAsset;
        AssetHandle FontHandle = 0;
        int FontFaceIndex = 0;
        glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        /// em 字号：1em = FontSize 设计像素。
        float FontSize = 48.0f;

        // 排版参数
        float Kerning = 0.0f;     // 字间距微调
        float LineSpacing = 0.0f; // 行间距微调
        TextHorizontalAlignment HorizontalAlignment = TextHorizontalAlignment::Left;
        TextVerticalAlignment VerticalAlignment = TextVerticalAlignment::Top;

        UITextComponent() = default;
        UITextComponent(const UITextComponent &) = default;
        UITextComponent(const std::string &text) : TextString(text)
        {
        }
    };

    struct UIButtonColorBlock
    {
        glm::vec4 NormalColor{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 HighlightedColor{0.9f, 0.9f, 0.9f, 1.0f};
        glm::vec4 PressedColor{0.7f, 0.7f, 0.7f, 1.0f};
        glm::vec4 DisabledColor{0.5f, 0.5f, 0.5f, 0.5f};
    };

    struct UIButtonComponent
    {
        bool Interactable = true;
        UIButtonColorBlock Colors;

        // 运行时状态，不序列化。
        bool IsPointerInside = false;
        bool IsPressed = false;
        bool WasClickedThisFrame = false;

        UIButtonComponent() = default;
        UIButtonComponent(const UIButtonComponent &) = default;

        glm::vec4 EvaluateTintMultiplier() const
        {
            if (!Interactable)
                return Colors.DisabledColor;
            if (IsPressed)
                return Colors.PressedColor;
            if (IsPointerInside)
                return Colors.HighlightedColor;
            return Colors.NormalColor;
        }

        glm::vec4 EvaluateEffectiveColor(const glm::vec4 &imageBaseColor) const
        {
            return imageBaseColor * EvaluateTintMultiplier();
        }
    };
#pragma endregion

    struct SoundPlayerComponent
    {
        AssetHandle SoundHandle = 0;
        Ref<SoundAsset> Sound;
        float Volume = 1.0f;
        bool Mute = false;
        bool Loop = false;
        bool PlayOnStart = false;

        // 运行时主轨，不序列化
        AudioVoiceHandle RuntimeVoiceHandle = AudioEngine::InvalidVoiceHandle;
        bool RuntimePaused = false;

        SoundPlayerComponent() = default;
        SoundPlayerComponent(const SoundPlayerComponent&) = default;

        float EvaluateEffectiveVolume() const
        {
            if (Mute)
                return 0.0f;
            if (Volume <= 0.0f)
                return 0.0f;
            if (Volume >= 1.0f)
                return 1.0f;
            return Volume;
        }
    };

}
