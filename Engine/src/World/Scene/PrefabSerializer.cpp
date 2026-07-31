#include "Hepch.h"
#include "PrefabSerializer.h"
#include "World/Scene/SceneSerializer.h"
#include "EngineCore/Core/Log.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    bool PrefabSerializer::Save(Entity entity, const std::filesystem::path& filepath,
                                const std::string& prefabName)
    {
        if (!entity)
            return false;

        std::string resolvedName = prefabName.empty() ? entity.GetName() : prefabName;

        YAML::Emitter output;
        output << YAML::BeginMap;
        output << YAML::Key << "AssetType" << YAML::Value << "Prefab";
        output << YAML::Key << "PrefabName" << YAML::Value << resolvedName;
        SceneSerializer::SerializeEntity(output, entity);
        output << YAML::EndMap;

        std::ofstream file(filepath);
        if (!file.is_open())
        {
            HIMII_CORE_ERROR("PrefabSerializer::Save: failed to open {0}", filepath.string());
            return false;
        }

        file << output.c_str();
        return true;
    }

    Entity PrefabSerializer::Instantiate(const Ref<Scene>& scene, const std::filesystem::path& filepath)
    {
        if (!scene || !std::filesystem::exists(filepath))
            return {};

        YAML::Node rootNode;
        try
        {
            rootNode = YAML::LoadFile(filepath.string());
        }
        catch (const YAML::Exception& exception)
        {
            HIMII_CORE_ERROR("PrefabSerializer::Instantiate: YAML error in {0}: {1}",
                             filepath.string(), exception.what());
            return {};
        }

        if (!rootNode["Entity"])
        {
            HIMII_CORE_ERROR("PrefabSerializer::Instantiate: missing Entity block in {0}",
                             filepath.string());
            return {};
        }

        UUID newEntityIdentifier;
        rootNode["Entity"] = static_cast<uint64_t>(newEntityIdentifier);

        SceneSerializer::DeserializeEntity(rootNode, scene);
        return scene->GetEntityByUUID(newEntityIdentifier);
    }
}
