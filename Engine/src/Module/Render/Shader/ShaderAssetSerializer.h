#pragma once

#include "Module/Render/Shader/ShaderAsset.h"
#include <filesystem>

namespace Himii
{
    class ShaderAssetSerializer
    {
    public:
        static constexpr const char *SourceSeparator = "---";

        static void Serialize(const std::filesystem::path &filepath, const Ref<ShaderAsset> &asset);
        static Ref<ShaderAsset> Deserialize(const std::filesystem::path &filepath);
        static std::string BuildDefaultSpatialLitTemplate();
    };
}
