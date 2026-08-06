#pragma once

#include "Module/Render/Mesh/MeshAsset.h"
#include "EngineCore/Core/Core.h"
#include <filesystem>

namespace Himii
{
    class HmeshAssetSerializer
    {
    public:
        static bool Serialize(const std::filesystem::path &filepath, const MeshAsset &meshAsset);
        static Ref<MeshAsset> Deserialize(const std::filesystem::path &filepath);
    };
}
