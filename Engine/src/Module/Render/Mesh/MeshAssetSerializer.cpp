#include "Hepch.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Mesh/GltfMeshGeometryLoader.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    std::filesystem::path MeshAssetSerializer::GetMeshMetaPath(const std::filesystem::path &meshSourcePath)
    {
        std::filesystem::path metaPath = meshSourcePath;
        metaPath.replace_extension(".hmeshmeta");
        return metaPath;
    }

    bool MeshAssetSerializer::WriteMeshMeta(const std::filesystem::path &meshSourcePath,
                                            const std::vector<AssetHandle> &defaultMaterialHandles)
    {
        const std::filesystem::path metaPath = GetMeshMetaPath(meshSourcePath);
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "DefaultMaterialHandles" << YAML::Value << YAML::BeginSeq;
        for (AssetHandle handle : defaultMaterialHandles)
            emitter << static_cast<uint64_t>(handle);
        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;

        std::ofstream outputFile(metaPath);
        if (!outputFile.is_open())
        {
            HIMII_CORE_ERROR("Failed to write mesh meta: {0}", metaPath.string());
            return false;
        }
        outputFile << emitter.c_str();
        return true;
    }

    bool MeshAssetSerializer::ReadMeshMeta(const std::filesystem::path &meshSourcePath,
                                           std::vector<AssetHandle> &outDefaultMaterialHandles)
    {
        outDefaultMaterialHandles.clear();
        const std::filesystem::path metaPath = GetMeshMetaPath(meshSourcePath);
        if (!std::filesystem::exists(metaPath))
            return false;

        try
        {
            YAML::Node data = YAML::LoadFile(metaPath.string());
            if (!data["DefaultMaterialHandles"] || !data["DefaultMaterialHandles"].IsSequence())
                return false;

            for (const auto &handleNode : data["DefaultMaterialHandles"])
                outDefaultMaterialHandles.push_back(handleNode.as<uint64_t>());
            return true;
        }
        catch (const YAML::Exception &exception)
        {
            HIMII_CORE_ERROR("Failed to read mesh meta '{0}': {1}", metaPath.string(), exception.what());
            return false;
        }
    }

    Ref<MeshAsset> MeshAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        Ref<MeshAsset> meshAsset = CreateRef<MeshAsset>();
        if (!LoadGltfMeshGeometry(filepath, *meshAsset))
            return nullptr;

        ReadMeshMeta(filepath, meshAsset->DefaultMaterialHandles);
        meshAsset->EnsureGpuResources();
        return meshAsset;
    }
}
