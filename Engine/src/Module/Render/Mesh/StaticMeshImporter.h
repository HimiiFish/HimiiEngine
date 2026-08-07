#pragma once

#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Resource/AssetManager.h"
#include <filesystem>

namespace Himii
{
    class StaticMeshImporter
    {
    public:
        /// 从源文件导入：烘焙 `.hmesh`、可选 companion、写入 meta；返回 `.hmesh` 的 Handle。
        static AssetHandle ImportFromSource(AssetManager &assetManager,
                                            const std::filesystem::path &relativeSourcePath,
                                            const StaticMeshImportSettings &importSettings);

        /// 对已有 `.hmesh` 重导；overrideSettings 为空则沿用 meta。
        static AssetHandle ReimportProduct(AssetManager &assetManager,
                                           const std::filesystem::path &relativeHmeshPath,
                                           const StaticMeshImportSettings *overrideSettings = nullptr,
                                           bool preserveExistingCompanionMaterials = false);

        static std::filesystem::path GetProductPathForSource(
                const std::filesystem::path &relativeSourcePath);
    };
}
