#include "Hepch.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Mesh/HmeshAssetSerializer.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    namespace
    {
        std::string NormalizePathExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return extension;
        }
    }

    std::filesystem::path MeshAssetSerializer::GetMeshMetaPath(const std::filesystem::path &meshAssetPath)
    {
        return std::filesystem::path(meshAssetPath.string() + ".meta");
    }

    bool MeshAssetSerializer::WriteStaticMeshMeta(const std::filesystem::path &hmeshAssetPath,
                                                  const StaticMeshImportSettings &importSettings,
                                                  const std::filesystem::path &relativeSourcePath,
                                                  const std::vector<AssetHandle> &defaultMaterialHandles,
                                                  const std::vector<std::string> &materialSlotNames)
    {
        const std::filesystem::path metaPath = GetMeshMetaPath(hmeshAssetPath);
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "SourceFile" << YAML::Value << relativeSourcePath.generic_string();
        emitter << YAML::Key << "UniformScale" << YAML::Value << importSettings.UniformScale;
        emitter << YAML::Key << "ImportMaterialsAndTextures" << YAML::Value
                << importSettings.ImportMaterialsAndTextures;
        emitter << YAML::Key << "CombineMeshes" << YAML::Value << importSettings.CombineMeshes;
        emitter << YAML::Key << "DefaultMaterialHandles" << YAML::Value << YAML::BeginSeq;
        for (AssetHandle handle : defaultMaterialHandles)
            emitter << static_cast<uint64_t>(handle);
        emitter << YAML::EndSeq;
        emitter << YAML::Key << "MaterialSlotNames" << YAML::Value << YAML::BeginSeq;
        for (const std::string &slotName : materialSlotNames)
            emitter << slotName;
        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;

        std::ofstream outputFile(metaPath);
        if (!outputFile.is_open())
        {
            HIMII_CORE_ERROR("Failed to write static mesh meta: {0}", metaPath.string());
            return false;
        }
        outputFile << emitter.c_str();
        return true;
    }

    bool MeshAssetSerializer::ReadStaticMeshMeta(const std::filesystem::path &hmeshAssetPath,
                                                   StaticMeshImportSettings &outImportSettings,
                                                   std::filesystem::path &outRelativeSourcePath,
                                                   std::vector<AssetHandle> &outDefaultMaterialHandles,
                                                   std::vector<std::string> &outMaterialSlotNames)
    {
        outImportSettings = StaticMeshImportSettings{};
        outRelativeSourcePath.clear();
        outDefaultMaterialHandles.clear();
        outMaterialSlotNames.clear();

        const std::filesystem::path metaPath = GetMeshMetaPath(hmeshAssetPath);
        if (!std::filesystem::exists(metaPath))
            return false;

        try
        {
            YAML::Node data = YAML::LoadFile(metaPath.string());
            if (data["SourceFile"])
                outRelativeSourcePath = data["SourceFile"].as<std::string>();
            if (data["UniformScale"])
                outImportSettings.UniformScale = data["UniformScale"].as<float>();
            if (data["ImportMaterialsAndTextures"])
                outImportSettings.ImportMaterialsAndTextures =
                        data["ImportMaterialsAndTextures"].as<bool>();
            if (data["CombineMeshes"])
                outImportSettings.CombineMeshes = data["CombineMeshes"].as<bool>();

            if (data["DefaultMaterialHandles"] && data["DefaultMaterialHandles"].IsSequence())
            {
                for (const auto &handleNode : data["DefaultMaterialHandles"])
                    outDefaultMaterialHandles.push_back(handleNode.as<uint64_t>());
            }

            if (data["MaterialSlotNames"] && data["MaterialSlotNames"].IsSequence())
            {
                for (const auto &nameNode : data["MaterialSlotNames"])
                    outMaterialSlotNames.push_back(nameNode.as<std::string>());
            }

            return true;
        }
        catch (const YAML::Exception &exception)
        {
            HIMII_CORE_ERROR("Failed to read static mesh meta '{0}': {1}", metaPath.string(),
                             exception.what());
            return false;
        }
    }

    bool MeshAssetSerializer::WriteMeshMeta(const std::filesystem::path &meshAssetPath,
                                            const std::vector<AssetHandle> &defaultMaterialHandles)
    {
        return WriteMeshMeta(meshAssetPath, defaultMaterialHandles, {});
    }

    bool MeshAssetSerializer::WriteMeshMeta(const std::filesystem::path &meshAssetPath,
                                            const std::vector<AssetHandle> &defaultMaterialHandles,
                                            const std::vector<std::string> &materialSlotNames)
    {
        StaticMeshImportSettings existingSettings;
        std::filesystem::path relativeSourcePath;
        std::vector<AssetHandle> ignoredHandles;
        std::vector<std::string> ignoredNames;
        ReadStaticMeshMeta(meshAssetPath, existingSettings, relativeSourcePath, ignoredHandles,
                           ignoredNames);
        return WriteStaticMeshMeta(meshAssetPath, existingSettings, relativeSourcePath,
                                   defaultMaterialHandles, materialSlotNames);
    }

    bool MeshAssetSerializer::ReadMeshMeta(const std::filesystem::path &meshAssetPath,
                                           std::vector<AssetHandle> &outDefaultMaterialHandles)
    {
        std::vector<std::string> ignoredSlotNames;
        StaticMeshImportSettings importSettings;
        std::filesystem::path relativeSourcePath;
        return ReadStaticMeshMeta(meshAssetPath, importSettings, relativeSourcePath,
                                  outDefaultMaterialHandles, ignoredSlotNames);
    }

    bool MeshAssetSerializer::ReadMeshMeta(const std::filesystem::path &meshAssetPath,
                                           std::vector<AssetHandle> &outDefaultMaterialHandles,
                                           std::vector<std::string> &outMaterialSlotNames)
    {
        StaticMeshImportSettings importSettings;
        std::filesystem::path relativeSourcePath;
        return ReadStaticMeshMeta(meshAssetPath, importSettings, relativeSourcePath,
                                  outDefaultMaterialHandles, outMaterialSlotNames);
    }

    Ref<MeshAsset> MeshAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        const std::string extension = NormalizePathExtension(filepath);
        if (extension != ".hmesh")
        {
            HIMII_CORE_ERROR(
                    "Mesh assets must use baked .hmesh product files. Reimport the source model: {0}",
                    filepath.string());
            return nullptr;
        }

        Ref<MeshAsset> meshAsset = HmeshAssetSerializer::Deserialize(filepath);
        if (!meshAsset)
            return nullptr;

        ReadMeshMeta(filepath, meshAsset->DefaultMaterialHandles, meshAsset->MaterialSlotNames);
        meshAsset->EnsureGpuResources();
        return meshAsset;
    }
}
