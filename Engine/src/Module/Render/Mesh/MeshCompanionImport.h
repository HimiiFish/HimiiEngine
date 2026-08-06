#pragma once

#include "Resource/AssetManager.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include <filesystem>

namespace Himii
{
    /// 按网格源扩展名写出 Texture / Material（不写 meta；meta 由 StaticMeshImporter 写入 `.hmesh.meta`）。
    bool ImportMeshCompanionAssets(AssetManager &assetManager,
                                   const std::filesystem::path &relativeMeshPath,
                                   MeshCompanionImportResult *outResult = nullptr);

    /// 磁盘上是否已有网格 meta（`.hmesh.meta` 或旧版源 `.ext.meta`）。
    bool MeshCompanionMetaExists(const std::filesystem::path &absoluteMeshOrSourcePath);
}
