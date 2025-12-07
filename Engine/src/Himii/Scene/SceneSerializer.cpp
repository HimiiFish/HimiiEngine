#include "Hepch.h"
#include "Himii/Scene/SceneSerializer.h"

#include "Himii/Scene/Entity.h"
#include "Himii/Scene/Components.h"
#include "Himii/Core/UUID.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

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

    static void SerializeEntity(YAML::Emitter &out, Entity entity)
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
            out << YAML::EndMap;

            out << YAML::Key << "Primary" << YAML::Value << cameraComp.Primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComp.FixedAspectRatio;

            out << YAML::EndMap;
			
        }
        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap;
            auto &spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << spriteRenderer.Color;
            if (spriteRenderer.Texture)
                out << YAML::Key << "TexturePath" << YAML::Value << spriteRenderer.Texture->GetPath();
            out << YAML::Key << "TilingFactor" << YAML::Value << spriteRenderer.TilingFactor;
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
            out << YAML::EndMap;
        }
        out << YAML::EndMap;
    }

    void SceneSerializer::Serialize(const std::string &filepath)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Scene" << YAML::Value << "Untitled";
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
                uint64_t uuid = entity["Entity"].as<uint64_t>();

                std::string name;
                auto tagComponent = entity["TagComponent"];
                if (tagComponent)
                    name = tagComponent["Tag"].as<std::string>();

                Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid,name);

                auto transformComponent = entity["TransformComponent"];
                if (transformComponent)
                {
                    auto &tc = deserializedEntity.GetComponent<TransformComponent>();
                    tc.Position = transformComponent["Position"].as<glm::vec3>();
                    tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
                    tc.Scale = transformComponent["Scale"].as<glm::vec3>();
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

                    cc.Primary = cameraComponent["Primary"].as<bool>();
                    cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
                }

                auto spriteRendererComponent = entity["SpriteRendererComponent"];
                if (spriteRendererComponent)
                {
                    auto &src = deserializedEntity.AddComponent<SpriteRendererComponent>();
                    src.Color = spriteRendererComponent["Color"].as<glm::vec4>();

                    if (spriteRendererComponent["TexturePath"])
                    {
                        std::string texturePath = spriteRendererComponent["TexturePath"].as<std::string>();
                        //auto path = Project::GetAssetFileSystemPath(texturePath);
                        src.Texture = Texture2D::Create(texturePath);
                    }
                    if (spriteRendererComponent["TilingFactor"])
                    {
                        src.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
                    }
                }
                auto circleRendererComponent = entity["CircleRendererComponent"];
                if (circleRendererComponent)
                {
                    auto &crc = deserializedEntity.AddComponent<CircleRendererComponent>();
                    crc.Color = circleRendererComponent["Color"].as<glm::vec4>();
                    crc.Thickness = circleRendererComponent["Thickness"].as<float>();
                    crc.Fade = circleRendererComponent["Fade"].as<float>();
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
                }
            }
            return true;
        }
    }
    bool SceneSerializer::DeserializeRuntime(const std::string &filepath)
    {
        return false;
    }
} // namespace Himii
