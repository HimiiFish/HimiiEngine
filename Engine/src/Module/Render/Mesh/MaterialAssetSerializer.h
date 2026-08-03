#pragma once

#include "Module/Render/Mesh/MaterialAsset.h"
#include "EngineCore/Core/Core.h"
#include <filesystem>

namespace Himii
{
    class MaterialAssetSerializer
    {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<MaterialAsset> &asset);
        static Ref<MaterialAsset> Deserialize(const std::filesystem::path &filepath);
    };
}
