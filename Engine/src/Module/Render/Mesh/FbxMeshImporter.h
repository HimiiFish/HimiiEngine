#pragma once

#include "Resource/AssetManager.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include <filesystem>

namespace Himii
{
    class FbxMeshImporter
    {
    public:
        static bool ImportCompanionAssets(AssetManager &assetManager,
                                          const std::filesystem::path &relativeSourcePath,
                                          MeshCompanionImportResult *outResult = nullptr);
    };
}
