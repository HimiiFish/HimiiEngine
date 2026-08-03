#pragma once

#include "Module/Render/Mesh/MeshAsset.h"
#include "EngineCore/Core/Core.h"
#include <filesystem>

namespace Himii
{
    class MeshAssetSerializer
    {
    public:
        static Ref<MeshAsset> Deserialize(const std::filesystem::path &filepath);
        static bool WriteMeshMeta(const std::filesystem::path &meshSourcePath,
                                  const std::vector<AssetHandle> &defaultMaterialHandles);
        static bool ReadMeshMeta(const std::filesystem::path &meshSourcePath,
                                 std::vector<AssetHandle> &outDefaultMaterialHandles);
        static std::filesystem::path GetMeshMetaPath(const std::filesystem::path &meshSourcePath);
    };
}
