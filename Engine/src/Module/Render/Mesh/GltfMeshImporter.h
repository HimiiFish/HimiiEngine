#pragma once

#include "Resource/AssetManager.h"
#include <filesystem>

namespace Himii
{
    /// 将 glTF/GLB 旁路生成 Texture / Material / 源文件.meta，并登记到 AssetManager。
    class GltfMeshImporter
    {
    public:
        /// 在 Mesh 主资产已写入 registry 后调用；返回是否成功写出旁路资产。
        static bool ImportCompanionAssets(AssetManager &assetManager,
                                          const std::filesystem::path &relativeMeshPath,
                                          AssetHandle meshHandle);
    };
}
