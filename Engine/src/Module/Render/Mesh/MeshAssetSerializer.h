#pragma once

#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "EngineCore/Core/Core.h"
#include <filesystem>

namespace Himii
{
    class MeshAssetSerializer
    {
    public:
        static Ref<MeshAsset> Deserialize(const std::filesystem::path &filepath);

        static std::filesystem::path GetMeshMetaPath(const std::filesystem::path &meshAssetPath);

        static bool WriteStaticMeshMeta(const std::filesystem::path &hmeshAssetPath,
                                        const StaticMeshImportSettings &importSettings,
                                        const std::filesystem::path &relativeSourcePath,
                                        const std::vector<AssetHandle> &defaultMaterialHandles,
                                        const std::vector<std::string> &materialSlotNames);

        static bool ReadStaticMeshMeta(const std::filesystem::path &hmeshAssetPath,
                                       StaticMeshImportSettings &outImportSettings,
                                       std::filesystem::path &outRelativeSourcePath,
                                       std::vector<AssetHandle> &outDefaultMaterialHandles,
                                       std::vector<std::string> &outMaterialSlotNames);

        /// 兼容旧 API：读写 DefaultMaterialHandles / MaterialSlotNames（不含 Import 设置）。
        static bool WriteMeshMeta(const std::filesystem::path &meshAssetPath,
                                  const std::vector<AssetHandle> &defaultMaterialHandles);

        static bool WriteMeshMeta(const std::filesystem::path &meshAssetPath,
                                  const std::vector<AssetHandle> &defaultMaterialHandles,
                                  const std::vector<std::string> &materialSlotNames);

        static bool ReadMeshMeta(const std::filesystem::path &meshAssetPath,
                                 std::vector<AssetHandle> &outDefaultMaterialHandles);

        static bool ReadMeshMeta(const std::filesystem::path &meshAssetPath,
                                 std::vector<AssetHandle> &outDefaultMaterialHandles,
                                 std::vector<std::string> &outMaterialSlotNames);
    };
}
