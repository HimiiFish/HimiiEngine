#include "EntitySnapshot.h"
#include "World/Scene/SceneSerializer.h"
#include <functional>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    std::string EntitySnapshot::Serialize(Entity entity)
    {
        YAML::Emitter output;
        SceneSerializer::SerializeEntity(output, entity);
        return std::string(output.c_str());
    }

    std::string EntitySnapshot::SerializeSubtree(const Ref<Scene>& scene, Entity rootEntity)
    {
        YAML::Emitter output;
        output << YAML::BeginSeq;

        std::function<void(Entity)> serializeRecursive = [&](Entity entity)
        {
            if (!entity)
                return;

            SceneSerializer::SerializeEntity(output, entity);
            for (UUID childIdentifier : scene->GetEntityChildren(entity))
                serializeRecursive(scene->GetEntityByUUID(childIdentifier));
        };

        serializeRecursive(rootEntity);
        output << YAML::EndSeq;
        return std::string(output.c_str());
    }

    Entity EntitySnapshot::Restore(const Ref<Scene>& scene, const std::string& yaml)
    {
        YAML::Node node = YAML::Load(yaml);
        if (!node["Entity"])
            return {};

        UUID entityIdentifier = node["Entity"].as<uint64_t>();
        Entity existingEntity = scene->GetEntityByUUID(entityIdentifier);
        if (existingEntity)
            scene->DestroyEntity(existingEntity);

        SceneSerializer::DeserializeEntity(node, scene);
        scene->RebuildHierarchyCache();
        return scene->GetEntityByUUID(entityIdentifier);
    }

    void EntitySnapshot::RestoreSubtree(const Ref<Scene>& scene, const std::string& yaml)
    {
        YAML::Node nodes = YAML::Load(yaml);
        if (!nodes.IsSequence())
            return;

        for (YAML::const_iterator iterator = nodes.begin(); iterator != nodes.end(); ++iterator)
        {
            YAML::Node node = *iterator;
            SceneSerializer::DeserializeEntity(node, scene);
        }

        scene->RebuildHierarchyCache();
    }
}
