#include "Hepch.h"
#include "World/Scene/SceneSerializer.h"

#include "World/Scene/Entity.h"
#include "World/Scene/Components.h"
#include "EngineCore/Core/UUID.h"
#include "Module/Script/ScriptEngine.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Resource/AssetManager.h"

#include <fstream>

namespace Himii
{
    static AssetHandle FindTextureAssetHandleByPath(const std::string& texturePath)
    {
        if (texturePath.empty() || !Project::GetActive())
            return 0;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return 0;

        const std::string normalizedSearchPath =
            std::filesystem::path(texturePath).generic_string();

        for (const auto& [handle, metadata] : assetManager->GetAssetRegistry())
        {
            if (metadata.Type != AssetType::Texture2D)
                continue;

            if (metadata.FilePath.generic_string() == normalizedSearchPath)
                return handle;

            const std::string absoluteAssetPath =
                Project::GetAssetFileSystemPath(metadata.FilePath).generic_string();
            if (absoluteAssetPath == normalizedSearchPath)
                return handle;
        }

        return 0;
    }

    static Ref<Texture2D> LoadTextureFromSerializedNode(const YAML::Node& componentNode)
    {
        if (!componentNode)
            return nullptr;

        auto assetManager = Project::GetActive() ? ResourceSystem::GetAssetManager() : nullptr;

        if (componentNode["TextureHandle"])
        {
            AssetHandle textureHandle = componentNode["TextureHandle"].as<uint64_t>();
            if (textureHandle != 0 && assetManager)
            {
                Ref<Asset> asset = assetManager->GetAsset(textureHandle);
                if (asset)
                    return std::static_pointer_cast<Texture2D>(asset);
            }
        }

        if (componentNode["TexturePath"])
        {
            const std::string texturePath = componentNode["TexturePath"].as<std::string>();
            if (assetManager)
            {
                AssetHandle existingHandle = FindTextureAssetHandleByPath(texturePath);
                if (existingHandle != 0)
                {
                    Ref<Asset> asset = assetManager->GetAsset(existingHandle);
                    if (asset)
                        return std::static_pointer_cast<Texture2D>(asset);
                }

                AssetHandle importedHandle = assetManager->ImportAsset(texturePath);
                if (importedHandle != 0)
                {
                    Ref<Asset> asset = assetManager->GetAsset(importedHandle);
                    if (asset)
                        return std::static_pointer_cast<Texture2D>(asset);
                }
            }

            return Texture2D::Create(texturePath);
        }

        return nullptr;
    }

    static AssetHandle ResolveTextureHandleForSerialize(const Ref<Texture2D>& texture,
                                                        AssetHandle explicitHandle)
    {
        if (explicitHandle != 0)
            return explicitHandle;

        if (!texture)
            return 0;

        AssetHandle handleFromRegistry = FindTextureAssetHandleByPath(texture->GetPath());
        return handleFromRegistry;
    }
}

namespace YAML
{
    template<>
    struct convert<glm::vec2> {
        static Node encode(const glm::vec2 &rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node &node, glm::vec2 &rhs)
        {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };
    template<>
    struct convert<glm::vec3> {
        static Node encode(const glm::vec3 &v)
        {
            Node node;
            node.push_back(v.x);
            node.push_back(v.y);
            node.push_back(v.z);
            return node;
        }
        static bool decode(const Node &node, glm::vec3 &v)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            return true;
        }
    };
    template<>
    struct convert<glm::vec4> {
        static Node encode(const glm::vec4 &v)
        {
            Node n;
            n.push_back(v.x);
            n.push_back(v.y);
            n.push_back(v.z);
            n.push_back(v.w);
            return n;
        }
        static bool decode(const Node &node, glm::vec4 &v)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;
            v.x = node[0].as<float>();
            v.y = node[1].as<float>();
            v.z = node[2].as<float>();
            v.w = node[3].as<float>();
            return true;
        }
    };
    template<>
    struct convert<Himii::UUID> {
        static Node encode(const Himii::UUID &uuid)
        {
            Node node;
            node.push_back((uint64_t)uuid);
            return node;
        }
        static bool decode(const Node &node, Himii::UUID& uuid)
        {
            uuid = node.as<uint64_t>();
            return true;
        }
    };
} // namespace YAML

namespace Himii
{
    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec2 &v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec3 &v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter &operator<<(YAML::Emitter &out, const glm::vec4 &v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z <<v.w<< YAML::EndSeq;
        return out;
    }

    SceneSerializer::SceneSerializer(const Ref<Scene> &scene) : m_Scene(scene)
    {
    }

    void SceneSerializer::SerializeEntity(YAML::Emitter &out, Entity entity)
    {
        HIMII_CORE_ASSERT(entity.HasComponent<IDComponent>());

        out << YAML::BeginMap; // Entity
        out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID(); 

        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap;
            auto &tag = entity.GetComponent<TagComponent>();
            out << YAML::Key << "Tag" << YAML::Value << tag.Tag;
            out << YAML::EndMap;
			
        }
        if (entity.HasComponent<RelationshipComponent>())
        {
            const auto& relationship = entity.GetComponent<RelationshipComponent>();
            if (relationship.Parent != 0 || relationship.SiblingIndex != 0)
            {
                out << YAML::Key << "RelationshipComponent";
                out << YAML::BeginMap;
                out << YAML::Key << "Parent" << YAML::Value << relationship.Parent;
                out << YAML::Key << "SiblingIndex" << YAML::Value << relationship.SiblingIndex;
                out << YAML::EndMap;
            }
        }
        if (entity.HasComponent<TransformComponent>())
        {
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap;
            auto &transform = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "Position" << YAML::Value << transform.Position;
            out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << transform.Scale;
            out << YAML::EndMap;
			
        }
        if (entity.HasComponent<CameraComponent>())
        {
            out << YAML::Key << "CameraComponent";
            out << YAML::BeginMap;
            auto &cameraComp = entity.GetComponent<CameraComponent>();

            out << YAML::Key << "Camera";
            out << YAML::BeginMap;
            out << YAML::Key << "ProjectionType" << YAML::Value << (int)cameraComp.Camera.GetProjectionType();
            out << YAML::Key << "PerspectiveFOV" << YAML::Value << cameraComp.Camera.GetPerspectiveVerticalFOV();
            out << YAML::Key << "PerspectiveNear" << YAML::Value << cameraComp.Camera.GetPerspectiveNearClip();
            out << YAML::Key << "PerspectiveFar" << YAML::Value << cameraComp.Camera.GetPerspectiveFarClip();
            out << YAML::Key << "OrthographicSize" << YAML::Value << cameraComp.Camera.GetOrthographicSize();
            out << YAML::Key << "OrthographicNear" << YAML::Value << cameraComp.Camera.GetOrthographicNearClip();
            out << YAML::Key << "OrthographicFar" << YAML::Value << cameraComp.Camera.GetOrthographicFarClip();
            out << YAML::Key << "BackgroundColor" << YAML::Value << cameraComp.Camera.GetBackgroundColor();
            out << YAML::EndMap;

            out << YAML::Key << "Primary" << YAML::Value << cameraComp.Primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComp.FixedAspectRatio;

            out << YAML::EndMap;
			
        }
        if (entity.HasComponent<ScriptComponent>())
        {
            out << YAML::Key << "ScriptComponent";
            out << YAML::BeginMap; // ScriptComponent

            auto &sc = entity.GetComponent<ScriptComponent>();
            out << YAML::Key << "ClassName" << YAML::Value << sc.ClassName;

            // Fields
            auto& fields = sc.Fields;
            if (!fields.empty())
            {
                out << YAML::Key << "ScriptFields" << YAML::Value << YAML::BeginSeq;
                for (const auto& [name, field] : fields)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "Name" << YAML::Value << name;
                    out << YAML::Key << "Type" << YAML::Value << (int)field.Type;
                    out << YAML::Key << "Data" << YAML::Value;

                    switch (field.Type)
                    {
                        case ScriptFieldType::Float: out << field.GetValue<float>(); break;
                        case ScriptFieldType::Double: out << field.GetValue<double>(); break;
                        case ScriptFieldType::Bool: out << field.GetValue<bool>(); break;
                        case ScriptFieldType::Char: out << field.GetValue<char>(); break;
                        case ScriptFieldType::Byte: out << field.GetValue<unsigned char>(); break;
                        case ScriptFieldType::Short: out << field.GetValue<short>(); break;
                        case ScriptFieldType::Int: out << field.GetValue<int>(); break;
                        case ScriptFieldType::Long: out << field.GetValue<long long>(); break;
                        case ScriptFieldType::UByte: out << field.GetValue<unsigned char>(); break;
                        case ScriptFieldType::UShort: out << field.GetValue<unsigned short>(); break;
                        case ScriptFieldType::UInt: out << field.GetValue<unsigned int>(); break;
                        case ScriptFieldType::ULong: out << field.GetValue<unsigned long long>(); break;
                        case ScriptFieldType::Vector2: out << field.GetValue<glm::vec2>(); break;
                        case ScriptFieldType::Vector3: out << field.GetValue<glm::vec3>(); break;
                        case ScriptFieldType::Vector4: out << field.GetValue<glm::vec4>(); break;
                        case ScriptFieldType::Entity: out << field.GetValue<UUID>(); break;
                        case ScriptFieldType::String: out << field.GetValue<std::string>(); break;
                        case ScriptFieldType::KeyCode: out << field.GetValue<int>(); break;
                    }
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }

            out << YAML::EndMap; // ScriptComponent
        }
        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap;
            auto &spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << spriteRenderer.Color;
            if (spriteRenderer.SpriteAssetHandle != 0)
                out << YAML::Key << "SpriteAssetHandle" << YAML::Value
                    << (uint64_t)spriteRenderer.SpriteAssetHandle;
            out << YAML::Key << "TilingFactor" << YAML::Value << spriteRenderer.TilingFactor;
            out << YAML::Key << "FlipHorizontal" << YAML::Value << spriteRenderer.FlipHorizontal;
            out << YAML::Key << "SortingLayer" << YAML::Value << spriteRenderer.SortingLayer;
            out << YAML::Key << "SortingOrder" << YAML::Value << spriteRenderer.SortingOrder;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<CircleRendererComponent>())
        {
            out << YAML::Key << "CircleRendererComponent";
            out << YAML::BeginMap;
            auto &circleRenderer = entity.GetComponent<CircleRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << circleRenderer.Color;
            out << YAML::Key << "Thickness" << YAML::Value << circleRenderer.Thickness;
            out << YAML::Key << "Fade" << YAML::Value << circleRenderer.Fade;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<MeshComponent>())
        {
            out << YAML::Key << "MeshComponent";
            out << YAML::BeginMap;
            auto& mesh = entity.GetComponent<MeshComponent>();
            out << YAML::Key << "Source" << YAML::Value << (int)mesh.Source;
            out << YAML::Key << "Type" << YAML::Value << (int)mesh.Type;
            out << YAML::Key << "Color" << YAML::Value << mesh.Color;
            out << YAML::Key << "MeshAssetHandle" << YAML::Value << (uint64_t)mesh.MeshAssetHandle;
            out << YAML::Key << "MaterialAssetHandles" << YAML::Value << YAML::BeginSeq;
            for (AssetHandle materialHandle : mesh.MaterialAssetHandles)
                out << (uint64_t)materialHandle;
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<LightComponent>())
        {
            out << YAML::Key << "LightComponent";
            out << YAML::BeginMap;
            auto &light = entity.GetComponent<LightComponent>();
            out << YAML::Key << "Type" << YAML::Value << static_cast<int>(light.Type);
            out << YAML::Key << "Color" << YAML::Value << light.Color;
            out << YAML::Key << "Intensity" << YAML::Value << light.Intensity;
            out << YAML::Key << "Enabled" << YAML::Value << light.Enabled;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<EnvironmentComponent>())
        {
            out << YAML::Key << "EnvironmentComponent";
            out << YAML::BeginMap;
            auto &environment = entity.GetComponent<EnvironmentComponent>();
            out << YAML::Key << "AmbientColor" << YAML::Value << environment.AmbientColor;
            out << YAML::Key << "AmbientIntensity" << YAML::Value << environment.AmbientIntensity;
            out << YAML::Key << "Enabled" << YAML::Value << environment.Enabled;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<Rigidbody2DComponent>())
        {
            out << YAML::Key << "Rigidbody2DComponent";
            out << YAML::BeginMap;
            auto &rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
            out << YAML::Key << "BodyType" << YAML::Value << (int)rigidbody2D.Type;
            out << YAML::Key << "FixedRotation" << YAML::Value << rigidbody2D.FixedRotation;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<BoxCollider2DComponent>())
        {
            out << YAML::Key << "BoxCollider2DComponent";
            out << YAML::BeginMap;
            auto &boxCollider2D = entity.GetComponent<BoxCollider2DComponent>();
            out << YAML::Key << "Offset" << YAML::Value << boxCollider2D.Offset;
            out << YAML::Key << "Size" << YAML::Value << boxCollider2D.Size;
            out << YAML::Key << "Density" << YAML::Value << boxCollider2D.Density;
            out << YAML::Key << "Friction" << YAML::Value << boxCollider2D.Friction;
            out << YAML::Key << "Restitution" << YAML::Value << boxCollider2D.Restitution;
            out << YAML::Key << "RestitutionThreshold" << YAML::Value << boxCollider2D.RestitutionThreshold;
            out << YAML::Key << "IsTrigger" << YAML::Value << boxCollider2D.IsTrigger;
            out << YAML::Key << "Layer" << YAML::Value << boxCollider2D.Layer;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<CircleCollider2DComponent>())
        {
            out << YAML::Key << "CircleCollider2DComponent";
            out << YAML::BeginMap;
            auto &circleCollider2D = entity.GetComponent<CircleCollider2DComponent>();
            out << YAML::Key << "Offset" << YAML::Value << circleCollider2D.Offset;
            out << YAML::Key << "Radius" << YAML::Value << circleCollider2D.Radius;
            out << YAML::Key << "Density" << YAML::Value << circleCollider2D.Density;
            out << YAML::Key << "Friction" << YAML::Value << circleCollider2D.Friction;
            out << YAML::Key << "Restitution" << YAML::Value << circleCollider2D.Restitution;
            out << YAML::Key << "RestitutionThreshold" << YAML::Value << circleCollider2D.RestitutionThreshold;
            out << YAML::Key << "IsTrigger" << YAML::Value << circleCollider2D.IsTrigger;
            out << YAML::Key << "Layer" << YAML::Value << circleCollider2D.Layer;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<SpriteAnimationComponent>())
        {
            out << YAML::Key << "SpriteAnimationComponent";
            out << YAML::BeginMap;
            auto &anim = entity.GetComponent<SpriteAnimationComponent>();
            out << YAML::Key << "AnimationHandle" << YAML::Value << (uint64_t)anim.AnimationHandle;
            out << YAML::Key << "CurrentAnimationName" << YAML::Value << anim.CurrentAnimationName;
            out << YAML::Key << "FrameRate" << YAML::Value << anim.FrameRate;
            out << YAML::Key << "Playing" << YAML::Value << anim.Playing;
            out << YAML::Key << "PreviewInScene" << YAML::Value << anim.PreviewInScene;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<TilemapComponent>())
        {
            out << YAML::Key << "TilemapComponent";
            out << YAML::BeginMap;
            auto &tm = entity.GetComponent<TilemapComponent>();
            out << YAML::Key << "TileMapHandle" << YAML::Value << (uint64_t)tm.TileMapHandle;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<TilemapCollider2DComponent>())
        {
            out << YAML::Key << "TilemapCollider2DComponent";
            out << YAML::BeginMap;
            auto& tilemapCollider = entity.GetComponent<TilemapCollider2DComponent>();
            out << YAML::Key << "Enabled" << YAML::Value << tilemapCollider.Enabled;
            out << YAML::Key << "MergeAdjacentCells" << YAML::Value << tilemapCollider.MergeAdjacentCells;
            out << YAML::EndMap;
        }
        if (entity.HasComponent<ParticleEmitterComponent>())
        {
            out << YAML::Key << "ParticleEmitterComponent";
            out << YAML::BeginMap;
            auto &pe = entity.GetComponent<ParticleEmitterComponent>();
            out << YAML::Key << "EmitterHandle" << YAML::Value << (uint64_t)pe.EmitterHandle;
            out << YAML::EndMap;
        }

        //------------UI-----------
        if (entity.HasComponent<RectTransformComponent>())
        {
            out << YAML::Key << "RectTransformComponent";
            out << YAML::BeginMap;
            auto &rectTransform = entity.GetComponent<RectTransformComponent>();
            out << YAML::Key << "AnchorMinimum" << YAML::Value << rectTransform.AnchorMinimum;
            out << YAML::Key << "AnchorMaximum" << YAML::Value << rectTransform.AnchorMaximum;
            out << YAML::Key << "Pivot" << YAML::Value << rectTransform.Pivot;
            out << YAML::Key << "AnchoredPosition" << YAML::Value << rectTransform.AnchoredPosition;
            out << YAML::Key << "SizeDelta" << YAML::Value << rectTransform.SizeDelta;
            out << YAML::Key << "RotationRadians" << YAML::Value << rectTransform.RotationRadians;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<CanvasComponent>())
        {
            out << YAML::Key << "CanvasComponent";
            out << YAML::BeginMap;
            auto& canvas = entity.GetComponent<CanvasComponent>();
            out << YAML::Key << "ScaleMode" << YAML::Value << static_cast<int>(canvas.ScaleMode);
            out << YAML::Key << "ReferenceResolution" << YAML::Value << canvas.ReferenceResolution;
            out << YAML::Key << "MatchWidthOrHeight" << YAML::Value << canvas.MatchWidthOrHeight;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<UIImageComponent>())
        {
            out << YAML::Key << "UIImageComponent";
            out << YAML::BeginMap;
            auto &uiImage = entity.GetComponent<UIImageComponent>();
            out << YAML::Key << "Color" << YAML::Value << uiImage.Color;
            const AssetHandle textureHandle =
                ResolveTextureHandleForSerialize(uiImage.Texture, uiImage.TextureHandle);
            if (textureHandle != 0)
                out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)textureHandle;
            else if (uiImage.Texture)
                out << YAML::Key << "TexturePath" << YAML::Value << uiImage.Texture->GetPath();
            out << YAML::EndMap;
        }

        if (entity.HasComponent<UITextComponent>())
        {
            out << YAML::Key << "UITextComponent";
            out << YAML::BeginMap; // UITextComponent
            auto &textComponent = entity.GetComponent<UITextComponent>();

            out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
            out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
            out << YAML::Key << "FontSize" << YAML::Value << textComponent.FontSize;
            out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;
            out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;
            out << YAML::Key << "HorizontalAlignment" << YAML::Value
                << static_cast<int>(textComponent.HorizontalAlignment);
            out << YAML::Key << "VerticalAlignment" << YAML::Value
                << static_cast<int>(textComponent.VerticalAlignment);

            // 序列化字体资产句柄；兼容旧 FontPath。
            if (textComponent.FontHandle)
                out << YAML::Key << "FontHandle" << YAML::Value << (uint64_t)textComponent.FontHandle;
            out << YAML::Key << "FontFaceIndex" << YAML::Value << textComponent.FontFaceIndex;
            if (textComponent.FontAsset)
                out << YAML::Key << "FontPath" << YAML::Value << textComponent.FontAsset->GetFilePath().string();
            else
                out << YAML::Key << "FontPath" << YAML::Value << "";

            out << YAML::EndMap; // UITextComponent
        }

        if (entity.HasComponent<UIButtonComponent>())
        {
            out << YAML::Key << "UIButtonComponent";
            out << YAML::BeginMap;
            auto& button = entity.GetComponent<UIButtonComponent>();
            out << YAML::Key << "Interactable" << YAML::Value << button.Interactable;
            out << YAML::Key << "NormalColor" << YAML::Value << button.Colors.NormalColor;
            out << YAML::Key << "HighlightedColor" << YAML::Value << button.Colors.HighlightedColor;
            out << YAML::Key << "PressedColor" << YAML::Value << button.Colors.PressedColor;
            out << YAML::Key << "DisabledColor" << YAML::Value << button.Colors.DisabledColor;
            out << YAML::EndMap;
        }

        if (entity.HasComponent<SoundPlayerComponent>())
        {
            out << YAML::Key << "SoundPlayerComponent";
            out << YAML::BeginMap;
            auto& soundPlayer = entity.GetComponent<SoundPlayerComponent>();
            out << YAML::Key << "SoundHandle" << YAML::Value << (uint64_t)soundPlayer.SoundHandle;
            out << YAML::Key << "Volume" << YAML::Value << soundPlayer.Volume;
            out << YAML::Key << "Mute" << YAML::Value << soundPlayer.Mute;
            out << YAML::Key << "Loop" << YAML::Value << soundPlayer.Loop;
            out << YAML::Key << "PlayOnStart" << YAML::Value << soundPlayer.PlayOnStart;
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    void SceneSerializer::Serialize(const std::string &filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
        out << YAML::Key << "SceneFormatVersion" << YAML::Value << 3;
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
        m_Scene->m_Registry.view<IDComponent>().each(
                [&](auto entityHandle, IDComponent &id)
                {
                    Entity entity{entityHandle, m_Scene.get()};
                    if (!entity)
                        return;
                    SerializeEntity(out, entity);
                });
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    void SceneSerializer::SerializeRuntime(const std::string &filepath)
    {
        // 当前不支持运行时序列化
    }

    bool SceneSerializer::Deserialize(const std::string &filepath)
    {
        YAML::Node data;
        try
        {
            data = YAML::LoadFile(filepath);
        }
        catch (YAML::ParserException &e)
        {
            HIMII_CORE_ERROR("Failed to load scene file '{0}'\n{1}", filepath, e.what());
            return false;
        }
        if (!data["Scene"])
            return false;

        std::string sceneName = data["Scene"].as<std::string>();

        auto entities = data["Entities"];
        if (entities)
        {
            for (auto entity: entities)
            {
                DeserializeEntity(entity, m_Scene);
            }
        }

        m_Scene->RebuildHierarchyCache();
        return true;
    }

    void SceneSerializer::DeserializeEntity(YAML::Node& entity, Ref<Scene> scene)
    {
        uint64_t uuid = entity["Entity"].as<uint64_t>();

        std::string name;
        auto tagComponent = entity["TagComponent"];
        if (tagComponent)
            name = tagComponent["Tag"].as<std::string>();

        const bool isUserInterfaceEntity =
                entity["RectTransformComponent"].IsDefined() || entity["UITransformComponent"].IsDefined();

        Entity deserializedEntity =
                isUserInterfaceEntity ? scene->CreateUIEntityWithUUID(uuid, name)
                                      : scene->CreateEntityWithUUID(uuid, name);

        auto transformComponent = entity["TransformComponent"];
        if (transformComponent && !isUserInterfaceEntity)
        {
            auto &transform = deserializedEntity.GetComponent<TransformComponent>();
            transform.Position = transformComponent["Position"].as<glm::vec3>();
            transform.Rotation = transformComponent["Rotation"].as<glm::vec3>();
            transform.Scale = transformComponent["Scale"].as<glm::vec3>();
        }

        auto relationshipComponent = entity["RelationshipComponent"];
        if (relationshipComponent)
        {
            auto& relationship = deserializedEntity.AddComponent<RelationshipComponent>();
            if (relationshipComponent["Parent"])
                relationship.Parent = relationshipComponent["Parent"].as<UUID>();
            if (relationshipComponent["SiblingIndex"])
                relationship.SiblingIndex = relationshipComponent["SiblingIndex"].as<uint32_t>();
        }

        auto cameraComponent = entity["CameraComponent"];
        if (cameraComponent)
        {
            auto &cc = deserializedEntity.AddComponent<CameraComponent>();

            auto cameraProps = cameraComponent["Camera"];
            cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

            cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
            cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
            cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());
                
            cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
            cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
            cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

            if(cameraProps["BackgroundColor"])
                cc.Camera.SetBackgroundColor(cameraProps["BackgroundColor"].as<glm::vec4>());

            cc.Primary = cameraComponent["Primary"].as<bool>();
            cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
        }

        auto scriptComponent = entity["ScriptComponent"];
        if (scriptComponent)
        {
            auto &sc = deserializedEntity.AddComponent<ScriptComponent>();
            sc.ClassName = scriptComponent["ClassName"].as<std::string>();

            auto scriptFields = scriptComponent["ScriptFields"];
            if (scriptFields)
            {
                auto& fieldMap = sc.Fields;
                for (auto scriptField : scriptFields)
                {
                    std::string name = scriptField["Name"].as<std::string>();
                    ScriptFieldType type = (ScriptFieldType)scriptField["Type"].as<int>();

                    ScriptFieldInstance fieldInstance;
                    fieldInstance.Type = type;

                    switch (type)
                    {
                        case ScriptFieldType::Float: fieldInstance.SetValue(scriptField["Data"].as<float>()); break;
                        case ScriptFieldType::Double: fieldInstance.SetValue(scriptField["Data"].as<double>()); break;
                        case ScriptFieldType::Bool: fieldInstance.SetValue(scriptField["Data"].as<bool>()); break;
                        case ScriptFieldType::Char: fieldInstance.SetValue(scriptField["Data"].as<char>()); break;
                        case ScriptFieldType::Byte: fieldInstance.SetValue(scriptField["Data"].as<unsigned char>()); break;
                        case ScriptFieldType::Short: fieldInstance.SetValue(scriptField["Data"].as<short>()); break;
                        case ScriptFieldType::Int: fieldInstance.SetValue(scriptField["Data"].as<int>()); break;
                        case ScriptFieldType::Long: fieldInstance.SetValue(scriptField["Data"].as<long long>()); break;
                        case ScriptFieldType::UByte: fieldInstance.SetValue(scriptField["Data"].as<unsigned char>()); break;
                        case ScriptFieldType::UShort: fieldInstance.SetValue(scriptField["Data"].as<unsigned short>()); break;
                        case ScriptFieldType::UInt: fieldInstance.SetValue(scriptField["Data"].as<unsigned int>()); break;
                        case ScriptFieldType::ULong: fieldInstance.SetValue(scriptField["Data"].as<unsigned long long>()); break;
                        case ScriptFieldType::Vector2: fieldInstance.SetValue(scriptField["Data"].as<glm::vec2>()); break;
                        case ScriptFieldType::Vector3: fieldInstance.SetValue(scriptField["Data"].as<glm::vec3>()); break;
                        case ScriptFieldType::Vector4: fieldInstance.SetValue(scriptField["Data"].as<glm::vec4>()); break;
                        case ScriptFieldType::Entity: fieldInstance.SetValue(scriptField["Data"].as<UUID>()); break;
                        case ScriptFieldType::String: fieldInstance.SetValue(scriptField["Data"].as<std::string>()); break;
                        case ScriptFieldType::KeyCode: fieldInstance.SetValue(scriptField["Data"].as<int>()); break;
                    }
                    fieldMap[name] = fieldInstance;
                }
            }
        }

        auto spriteRendererComponent = entity["SpriteRendererComponent"];
        if (spriteRendererComponent)
        {
            auto &src = deserializedEntity.AddComponent<SpriteRendererComponent>();
            src.Color = spriteRendererComponent["Color"].as<glm::vec4>();

            if (spriteRendererComponent["SpriteAssetHandle"])
                src.SpriteAssetHandle = spriteRendererComponent["SpriteAssetHandle"].as<uint64_t>();
            else if (spriteRendererComponent["SpriteHandle"])
                src.SpriteAssetHandle = spriteRendererComponent["SpriteHandle"].as<uint64_t>();

            if (src.SpriteAssetHandle == 0)
            {
                AssetHandle legacyTextureHandle = 0;
                if (spriteRendererComponent["TextureHandle"])
                    legacyTextureHandle = spriteRendererComponent["TextureHandle"].as<uint64_t>();
                else
                {
                    Ref<Texture2D> legacyTexture = LoadTextureFromSerializedNode(spriteRendererComponent);
                    if (legacyTexture)
                        legacyTextureHandle = FindTextureAssetHandleByPath(legacyTexture->GetPath());
                }

                if (legacyTextureHandle != 0)
                {
                    if (auto assetManager = ResourceSystem::GetAssetManager())
                    {
                        if (assetManager->IsAssetHandleValid(legacyTextureHandle))
                        {
                            src.SpriteAssetHandle =
                                assetManager->GetDefaultSpriteHandleForTexture(legacyTextureHandle);
                        }
                    }
                }
            }

            if (spriteRendererComponent["TilingFactor"])
                src.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
            if (spriteRendererComponent["FlipHorizontal"])
                src.FlipHorizontal = spriteRendererComponent["FlipHorizontal"].as<bool>();
            if (spriteRendererComponent["SortingLayer"])
                src.SortingLayer = spriteRendererComponent["SortingLayer"].as<int>();
            if (spriteRendererComponent["SortingOrder"])
                src.SortingOrder = spriteRendererComponent["SortingOrder"].as<int>();
        }
        auto circleRendererComponent = entity["CircleRendererComponent"];
        if (circleRendererComponent)
        {
            auto &crc = deserializedEntity.AddComponent<CircleRendererComponent>();
            crc.Color = circleRendererComponent["Color"].as<glm::vec4>();
            crc.Thickness = circleRendererComponent["Thickness"].as<float>();
            crc.Fade = circleRendererComponent["Fade"].as<float>();
        }

        auto meshComponent = entity["MeshComponent"];
        if (meshComponent)
        {
            auto& mc = deserializedEntity.AddComponent<MeshComponent>();
            mc.Color = meshComponent["Color"].as<glm::vec4>();
            if (meshComponent["Type"])
                mc.Type = (MeshComponent::MeshType)meshComponent["Type"].as<int>();
            if (meshComponent["Source"])
                mc.Source = (MeshComponent::MeshSource)meshComponent["Source"].as<int>();
            if (meshComponent["MeshAssetHandle"])
                mc.MeshAssetHandle = meshComponent["MeshAssetHandle"].as<uint64_t>();
            if (meshComponent["MaterialAssetHandles"] && meshComponent["MaterialAssetHandles"].IsSequence())
            {
                mc.MaterialAssetHandles.clear();
                for (const auto &handleNode : meshComponent["MaterialAssetHandles"])
                    mc.MaterialAssetHandles.push_back(handleNode.as<uint64_t>());
            }
        }

        auto lightComponent = entity["LightComponent"];
        if (lightComponent)
        {
            auto &light = deserializedEntity.AddComponent<LightComponent>();
            if (lightComponent["Type"])
                light.Type = static_cast<LightType>(lightComponent["Type"].as<int>());
            if (lightComponent["Color"])
                light.Color = lightComponent["Color"].as<glm::vec4>();
            if (lightComponent["Intensity"])
                light.Intensity = lightComponent["Intensity"].as<float>();
            if (lightComponent["Enabled"])
                light.Enabled = lightComponent["Enabled"].as<bool>();
        }

        auto environmentComponent = entity["EnvironmentComponent"];
        if (environmentComponent)
        {
            auto &environment = deserializedEntity.AddComponent<EnvironmentComponent>();
            if (environmentComponent["AmbientColor"])
                environment.AmbientColor = environmentComponent["AmbientColor"].as<glm::vec4>();
            if (environmentComponent["AmbientIntensity"])
                environment.AmbientIntensity = environmentComponent["AmbientIntensity"].as<float>();
            if (environmentComponent["Enabled"])
                environment.Enabled = environmentComponent["Enabled"].as<bool>();
        }

        auto rigidbody2DComponent = entity["Rigidbody2DComponent"];
        if (rigidbody2DComponent)
        {
            auto &rbc = deserializedEntity.AddComponent<Rigidbody2DComponent>();
            rbc.Type = (Rigidbody2DComponent::BodyType)rigidbody2DComponent["BodyType"].as<int>();
            rbc.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
        }
        auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
        if (boxCollider2DComponent)
        {
            auto &bcc = deserializedEntity.AddComponent<BoxCollider2DComponent>();
            bcc.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
            bcc.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
            bcc.Density = boxCollider2DComponent["Density"].as<float>();
            bcc.Friction = boxCollider2DComponent["Friction"].as<float>();
            bcc.Restitution = boxCollider2DComponent["Restitution"].as<float>();
            bcc.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
            if (boxCollider2DComponent["IsTrigger"])
                bcc.IsTrigger = boxCollider2DComponent["IsTrigger"].as<bool>();
            if (boxCollider2DComponent["Layer"])
                bcc.Layer = boxCollider2DComponent["Layer"].as<int>();
        }
        auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
        if (circleCollider2DComponent)
        {
            auto &ccc = deserializedEntity.AddComponent<CircleCollider2DComponent>();
            ccc.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
            ccc.Radius = circleCollider2DComponent["Radius"].as<float>();
            ccc.Density = circleCollider2DComponent["Density"].as<float>();
            ccc.Friction = circleCollider2DComponent["Friction"].as<float>();
            ccc.Restitution = circleCollider2DComponent["Restitution"].as<float>();
            ccc.RestitutionThreshold = circleCollider2DComponent["RestitutionThreshold"].as<float>();
            if (circleCollider2DComponent["IsTrigger"])
                ccc.IsTrigger = circleCollider2DComponent["IsTrigger"].as<bool>();
            if (circleCollider2DComponent["Layer"])
                ccc.Layer = circleCollider2DComponent["Layer"].as<int>();
        }
        auto spriteAnimationComponent = entity["SpriteAnimationComponent"];
        if (spriteAnimationComponent)
        {
            auto &sac = deserializedEntity.AddComponent<SpriteAnimationComponent>();
            sac.AnimationHandle = spriteAnimationComponent["AnimationHandle"].as<uint64_t>();
            if (spriteAnimationComponent["CurrentAnimationName"])
                sac.CurrentAnimationName =
                        spriteAnimationComponent["CurrentAnimationName"].as<std::string>();
            if (spriteAnimationComponent["FrameRate"])
                sac.FrameRate = spriteAnimationComponent["FrameRate"].as<float>();
            if (spriteAnimationComponent["Playing"])
                sac.Playing = spriteAnimationComponent["Playing"].as<bool>();
            if (spriteAnimationComponent["PreviewInScene"])
                sac.PreviewInScene = spriteAnimationComponent["PreviewInScene"].as<bool>();
        }

        auto tilemapComponent = entity["TilemapComponent"];
        if (tilemapComponent)
        {
            auto &tm = deserializedEntity.AddComponent<TilemapComponent>();
            if (tilemapComponent["TileMapHandle"])
                tm.TileMapHandle = tilemapComponent["TileMapHandle"].as<uint64_t>();
        }

        auto tilemapColliderComponent = entity["TilemapCollider2DComponent"];
        if (tilemapColliderComponent)
        {
            auto& tilemapCollider = deserializedEntity.AddComponent<TilemapCollider2DComponent>();
            if (tilemapColliderComponent["Enabled"])
                tilemapCollider.Enabled = tilemapColliderComponent["Enabled"].as<bool>();
            if (tilemapColliderComponent["MergeAdjacentCells"])
                tilemapCollider.MergeAdjacentCells =
                        tilemapColliderComponent["MergeAdjacentCells"].as<bool>();
        }

        auto particleEmitterComponent = entity["ParticleEmitterComponent"];
        if (particleEmitterComponent)
        {
            auto &pe = deserializedEntity.AddComponent<ParticleEmitterComponent>();
            if (particleEmitterComponent["EmitterHandle"])
                pe.EmitterHandle = particleEmitterComponent["EmitterHandle"].as<uint64_t>();
        }


        //-----------UI----------
        auto rectTransformComponent = entity["RectTransformComponent"];
        auto legacyUserInterfaceTransformComponent = entity["UITransformComponent"];
        if (rectTransformComponent)
        {
            auto &rectTransform = deserializedEntity.GetComponent<RectTransformComponent>();
            rectTransform.AnchorMinimum = rectTransformComponent["AnchorMinimum"].as<glm::vec2>();
            rectTransform.AnchorMaximum = rectTransformComponent["AnchorMaximum"].as<glm::vec2>();
            rectTransform.Pivot = rectTransformComponent["Pivot"].as<glm::vec2>();
            rectTransform.AnchoredPosition = rectTransformComponent["AnchoredPosition"].as<glm::vec2>();
            rectTransform.SizeDelta = rectTransformComponent["SizeDelta"].as<glm::vec2>();
            if (rectTransformComponent["RotationRadians"])
                rectTransform.RotationRadians =
                        rectTransformComponent["RotationRadians"].as<float>();
            else if (rectTransformComponent["Rotation"])
                rectTransform.RotationRadians =
                        rectTransformComponent["Rotation"].as<glm::vec3>().z;
            rectTransform.ResolvedSize = rectTransform.SizeDelta;
        }
        else if (legacyUserInterfaceTransformComponent)
        {
            auto &rectTransform = deserializedEntity.GetComponent<RectTransformComponent>();
            const glm::vec3 legacyPosition =
                    legacyUserInterfaceTransformComponent["Position"].as<glm::vec3>();
            rectTransform.AnchorMinimum = glm::vec2(0.5f);
            rectTransform.AnchorMaximum = glm::vec2(0.5f);
            rectTransform.Pivot = glm::vec2(0.5f);
            rectTransform.AnchoredPosition = glm::vec2(legacyPosition);
            rectTransform.SizeDelta = legacyUserInterfaceTransformComponent["Size"].as<glm::vec2>();
            rectTransform.RotationRadians =
                    legacyUserInterfaceTransformComponent["Rotation"].as<glm::vec3>().z;
            rectTransform.ResolvedSize = rectTransform.SizeDelta;
        }

        auto canvasComponent = entity["CanvasComponent"];
        if (canvasComponent)
        {
            auto& canvas = deserializedEntity.AddComponent<CanvasComponent>();
            if (canvasComponent["ScaleMode"])
                canvas.ScaleMode = static_cast<CanvasScaleMode>(canvasComponent["ScaleMode"].as<int>());
            if (canvasComponent["ReferenceResolution"])
                canvas.ReferenceResolution = canvasComponent["ReferenceResolution"].as<glm::vec2>();
            if (canvasComponent["MatchWidthOrHeight"])
                canvas.MatchWidthOrHeight = canvasComponent["MatchWidthOrHeight"].as<float>();
            scene->SyncCanvasReferenceResolutionToTransform(deserializedEntity);
        }

        auto uiImageComponent = entity["UIImageComponent"];
        if (uiImageComponent)
        {
            auto &uiImage = deserializedEntity.AddComponent<UIImageComponent>();
            uiImage.Color = uiImageComponent["Color"].as<glm::vec4>();
            uiImage.Texture = LoadTextureFromSerializedNode(uiImageComponent);
            if (uiImageComponent["TextureHandle"])
                uiImage.TextureHandle = uiImageComponent["TextureHandle"].as<uint64_t>();
            else if (uiImage.Texture)
                uiImage.TextureHandle = FindTextureAssetHandleByPath(uiImage.Texture->GetPath());
        }

        auto uiTextComponent = entity["UITextComponent"];
        if (uiTextComponent)
        {
            // 如果实体还没有这个组件（通常新创建的 UIEntity 只有 UITransform），则添加
            auto &textComp = deserializedEntity.AddComponent<UITextComponent>();

            textComp.TextString = uiTextComponent["TextString"].as<std::string>();
            textComp.Color = uiTextComponent["Color"].as<glm::vec4>();

            // 使用 IsDefined 检查可选字段，防止老版本场景文件崩溃
            if (uiTextComponent["FontSize"])
                textComp.FontSize = uiTextComponent["FontSize"].as<float>();
            if (uiTextComponent["Kerning"])
                textComp.Kerning = uiTextComponent["Kerning"].as<float>();
            if (uiTextComponent["LineSpacing"])
                textComp.LineSpacing = uiTextComponent["LineSpacing"].as<float>();
            if (uiTextComponent["HorizontalAlignment"])
                textComp.HorizontalAlignment = static_cast<TextHorizontalAlignment>(
                        uiTextComponent["HorizontalAlignment"].as<int>());
            if (uiTextComponent["VerticalAlignment"])
                textComp.VerticalAlignment = static_cast<TextVerticalAlignment>(
                        uiTextComponent["VerticalAlignment"].as<int>());

            // 反序列化并加载字体
            if (uiTextComponent["FontHandle"])
            {
                textComp.FontHandle = uiTextComponent["FontHandle"].as<uint64_t>();
                if (textComp.FontHandle && Project::GetActive())
                {
                    if (auto assetManager = ResourceSystem::GetAssetManager())
                    {
                        if (Ref<Asset> asset = assetManager->GetAsset(textComp.FontHandle))
                            textComp.FontAsset = std::dynamic_pointer_cast<Font>(asset);
                    }
                }
            }
            if (uiTextComponent["FontFaceIndex"])
                textComp.FontFaceIndex = uiTextComponent["FontFaceIndex"].as<int>();

            if (!textComp.FontAsset && uiTextComponent["FontPath"])
            {
                std::string fontPath = uiTextComponent["FontPath"].as<std::string>();
                if (!fontPath.empty())
                {
                    FontSpecification specification;
                    specification.FilePath = fontPath;
                    specification.FaceIndex = textComp.FontFaceIndex;
                    textComp.FontAsset = CreateRef<Font>(specification);
                }
                else
                {
                    textComp.FontAsset = Font::GetDefault();
                }
            }
            if (!textComp.FontAsset)
                textComp.FontAsset = Font::GetDefault();
        }

        auto uiButtonComponent = entity["UIButtonComponent"];
        if (uiButtonComponent)
        {
            auto& button = deserializedEntity.AddComponent<UIButtonComponent>();
            if (uiButtonComponent["Interactable"])
                button.Interactable = uiButtonComponent["Interactable"].as<bool>();
            if (uiButtonComponent["NormalColor"])
                button.Colors.NormalColor = uiButtonComponent["NormalColor"].as<glm::vec4>();
            if (uiButtonComponent["HighlightedColor"])
                button.Colors.HighlightedColor = uiButtonComponent["HighlightedColor"].as<glm::vec4>();
            if (uiButtonComponent["PressedColor"])
                button.Colors.PressedColor = uiButtonComponent["PressedColor"].as<glm::vec4>();
            if (uiButtonComponent["DisabledColor"])
                button.Colors.DisabledColor = uiButtonComponent["DisabledColor"].as<glm::vec4>();
        }

        auto soundPlayerComponent = entity["SoundPlayerComponent"];
        if (soundPlayerComponent)
        {
            auto& soundPlayer = deserializedEntity.AddComponent<SoundPlayerComponent>();
            if (soundPlayerComponent["SoundHandle"])
                soundPlayer.SoundHandle = soundPlayerComponent["SoundHandle"].as<uint64_t>();
            if (soundPlayerComponent["Volume"])
                soundPlayer.Volume = soundPlayerComponent["Volume"].as<float>();
            if (soundPlayerComponent["Mute"])
                soundPlayer.Mute = soundPlayerComponent["Mute"].as<bool>();
            if (soundPlayerComponent["Loop"])
                soundPlayer.Loop = soundPlayerComponent["Loop"].as<bool>();
            if (soundPlayerComponent["PlayOnStart"])
                soundPlayer.PlayOnStart = soundPlayerComponent["PlayOnStart"].as<bool>();

            if (soundPlayer.SoundHandle && Project::GetActive())
            {
                auto assetManager = Project::GetActive()->GetAssetManager();
                if (assetManager && assetManager->IsAssetHandleValid(soundPlayer.SoundHandle))
                    soundPlayer.Sound = std::dynamic_pointer_cast<SoundAsset>(
                            assetManager->GetAsset(soundPlayer.SoundHandle));
            }
        }
    }
    
    bool SceneSerializer::DeserializeRuntime(const std::string &filepath)
    {
        return false;
    }
} // namespace Himii
