#pragma once

#include "Module/Render/Mesh/MeshAsset.h"
#include <filesystem>

namespace Himii
{
    /// 从 glTF/GLB 填充静态网格几何（不做材质旁路写出）。
    bool LoadGltfMeshGeometry(const std::filesystem::path &filesystemPath, MeshAsset &outMeshAsset);
}
