#pragma once

#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include <filesystem>

namespace Himii
{
    /// 从网格源文件填充几何；应用统一缩放；CombineMeshes 暂与现有加载器行为一致。
    bool LoadMeshGeometryFromSource(const std::filesystem::path &absoluteSourcePath,
                                    const StaticMeshImportSettings &importSettings,
                                    MeshAsset &outMeshAsset);
}
