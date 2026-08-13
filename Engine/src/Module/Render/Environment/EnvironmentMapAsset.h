#pragma once

#include "Resource/Asset.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace Himii
{
    struct EnvironmentImportSettings
    {
        uint32_t CubemapResolution = 128;
        uint32_t IrradianceResolution = 32;
        uint32_t PrefilterResolution = 128;
        uint32_t PrefilterMipCount = 5;
        uint32_t BrdfLookupSize = 256;
        std::string SourceContentHash;
    };

    /// HDR 环境源资产（equirect）；卷积结果不进本资产，而在 Saved/EnvironmentBake。
    class EnvironmentMapAsset : public Asset
    {
    public:
        std::filesystem::path SourceFilePath;
        EnvironmentImportSettings ImportSettings;

        AssetType GetType() const override { return AssetType::EnvironmentMap; }
    };

    class EnvironmentMapImportSerializer
    {
    public:
        static std::filesystem::path GetMetaPath(const std::filesystem::path &environmentFilesystemPath);
        static bool MetaExists(const std::filesystem::path &environmentFilesystemPath);
        static bool Deserialize(const std::filesystem::path &environmentFilesystemPath,
                                EnvironmentImportSettings &outSettings);
        static bool Serialize(const std::filesystem::path &environmentFilesystemPath,
                              const EnvironmentImportSettings &settings);
        static std::string ComputeSourceContentHash(const std::filesystem::path &environmentFilesystemPath);
        static void EnsureDefaultMeta(const std::filesystem::path &environmentFilesystemPath);
    };
}
