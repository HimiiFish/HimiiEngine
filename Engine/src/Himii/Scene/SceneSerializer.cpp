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
            out << YAML::Key << "Tag" << YAML::Value << tag.name;
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
            out << YAML::Key << "ProjectionType" << YAML::Value << (int)cameraComp.camera.GetProjectionType();
            out << YAML::Key << "PerspectiveFOV" << YAML::Value << cameraComp.camera.GetPerspectiveVerticalFOV();
            out << YAML::Key << "PerspectiveNear" << YAML::Value << cameraComp.camera.GetPerspectiveNearClip();
            out << YAML::Key << "PerspectiveFar" << YAML::Value << cameraComp.camera.GetPerspectiveFarClip();
            out << YAML::Key << "OrthographicSize" << YAML::Value << cameraComp.camera.GetOrthographicSize();
            out << YAML::Key << "OrthographicNear" << YAML::Value << cameraComp.camera.GetOrthographicNearClip();
            out << YAML::Key << "OrthographicFar" << YAML::Value << cameraComp.camera.GetOrthographicFarClip();
            out << YAML::EndMap;

            out << YAML::Key << "Primary" << YAML::Value << cameraComp.primary;
            out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComp.fixedAspectRatio;

            out << YAML::EndMap;
			
        }
        if (entity.HasComponent<SpriteRendererComponent>())
        {
            out << YAML::Key << "SpriteRendererComponent";
            out << YAML::BeginMap;
            auto &spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
            out << YAML::Key << "Color" << YAML::Value << spriteRenderer.color;
            out << YAML::Key << "TilingFactor" << YAML::Value << spriteRenderer.tilingFactor;
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
                    cc.camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

                    cc.camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
                    cc.camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
                    cc.camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

                    cc.camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
                    cc.camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
                    cc.camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

                    cc.primary = cameraComponent["Primary"].as<bool>();
                    cc.fixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
                }

                auto spriteRendererComponent = entity["SpriteRendererComponent"];
                if (spriteRendererComponent)
                {
                    auto &src = deserializedEntity.AddComponent<SpriteRendererComponent>();
                    src.color = spriteRendererComponent["Color"].as<glm::vec4>();
                    if (spriteRendererComponent["TilingFactor"])
                    {
                        src.tilingFactor = spriteRendererComponent["TilingFactor"].as<float>();
                    }
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
