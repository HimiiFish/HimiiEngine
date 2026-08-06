#pragma once

#include "Resource/Asset.h"
#include <string>
#include <vector>

namespace Himii
{
    struct StaticMeshImportSettings
    {
        float UniformScale = 1.0f;
        bool ImportMaterialsAndTextures = true;
        bool CombineMeshes = true;
    };

    struct MeshCompanionImportResult
    {
        std::vector<AssetHandle> MaterialHandles;
        std::vector<std::string> MaterialSlotNames;
    };

    /// 可导入为静态网格产品的源扩展名（小写，含点）。
    bool IsStaticMeshSourceExtension(const std::string &extensionLowercase);

    /// 已烘焙的静态网格产品扩展名。
    inline bool IsStaticMeshProductExtension(const std::string &extensionLowercase)
    {
        return extensionLowercase == ".hmesh";
    }

} // namespace Himii
