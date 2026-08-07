#pragma once

#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include <filesystem>
#include <functional>

namespace Himii
{
    struct StaticMeshImportDialogState
    {
        bool Open = false;
        bool IsReimport = false;
        StaticMeshImportSettings Settings{};
        std::filesystem::path RelativeSourcePath;
        std::filesystem::path RelativeHmeshPath;

        bool HasExistingCompanionMaterials = false;
        bool AwaitingMaterialReimportChoice = false;
        bool PreserveCompanionMaterialsOnReimport = false;
    };

    /// 返回 true 表示用户确认并应执行导入。
    bool DrawStaticMeshImportDialog(StaticMeshImportDialogState &dialogState);

} // namespace Himii
