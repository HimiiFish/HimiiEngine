#include "Hepch.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    static void WriteVector4(YAML::Emitter &emitter, const char *key, const glm::vec4 &value)
    {
        emitter << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq << value.x << value.y
                << value.z << value.w << YAML::EndSeq;
    }

    static glm::vec4 ReadVector4(const YAML::Node &node, const char *key, const glm::vec4 &defaultValue)
    {
        if (!node[key] || !node[key].IsSequence() || node[key].size() < 4)
            return defaultValue;
        return {node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>(),
                node[key][3].as<float>()};
    }

    void MaterialAssetSerializer::Serialize(const std::filesystem::path &filepath, const Ref<MaterialAsset> &asset)
    {
        if (!asset)
            return;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetType" << YAML::Value << "Material";
        emitter << YAML::Key << "Handle" << YAML::Value << static_cast<uint64_t>(asset->Handle);
        emitter << YAML::Key << "ShadingMode" << YAML::Value << static_cast<int>(asset->ShadingMode);
        WriteVector4(emitter, "AlbedoColor", asset->AlbedoColor);
        emitter << YAML::Key << "AlbedoTextureHandle" << YAML::Value
                << static_cast<uint64_t>(asset->AlbedoTextureHandle);
        if (!asset->AlbedoTextureRelativePath.empty())
            emitter << YAML::Key << "AlbedoTextureRelativePath" << YAML::Value
                    << asset->AlbedoTextureRelativePath;
        emitter << YAML::Key << "Specular" << YAML::Value << asset->Specular;
        emitter << YAML::Key << "Shininess" << YAML::Value << asset->Shininess;
        emitter << YAML::EndMap;

        std::ofstream outputFile(filepath);
        outputFile << emitter.c_str();
    }

    Ref<MaterialAsset> MaterialAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream inputStream(filepath);
            if (!inputStream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open MaterialAsset file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream stringStream;
            stringStream << inputStream.rdbuf();
            YAML::Node data = YAML::Load(stringStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "Material")
            {
                HIMII_CORE_ERROR("Invalid MaterialAsset: {0}", filepath.string());
                return nullptr;
            }

            Ref<MaterialAsset> asset = CreateRef<MaterialAsset>();
            if (data["Handle"])
                asset->Handle = data["Handle"].as<uint64_t>();
            if (data["ShadingMode"])
                asset->ShadingMode = static_cast<MaterialShadingMode>(data["ShadingMode"].as<int>());
            else
                asset->ShadingMode = MaterialShadingMode::Lit;
            asset->AlbedoColor = ReadVector4(data, "AlbedoColor", {1.0f, 1.0f, 1.0f, 1.0f});
            if (data["AlbedoTextureHandle"])
                asset->AlbedoTextureHandle = data["AlbedoTextureHandle"].as<uint64_t>();
            if (data["AlbedoTextureRelativePath"])
                asset->AlbedoTextureRelativePath = data["AlbedoTextureRelativePath"].as<std::string>();
            if (data["Specular"])
                asset->Specular = data["Specular"].as<float>();
            if (data["Shininess"])
                asset->Shininess = data["Shininess"].as<float>();
            return asset;
        }
        catch (const YAML::Exception &exception)
        {
            HIMII_CORE_ERROR("Failed to deserialize MaterialAsset '{0}': {1}", filepath.string(),
                             exception.what());
            return nullptr;
        }
    }
}
