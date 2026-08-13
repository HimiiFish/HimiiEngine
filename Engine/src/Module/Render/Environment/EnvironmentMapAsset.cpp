#include "Hepch.h"
#include "Module/Render/Environment/EnvironmentMapAsset.h"

#include "EngineCore/Core/Log.h"

#include <fstream>
#include <iomanip>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace Himii
{
    std::filesystem::path EnvironmentMapImportSerializer::GetMetaPath(
            const std::filesystem::path &environmentFilesystemPath)
    {
        return environmentFilesystemPath.string() + ".meta";
    }

    bool EnvironmentMapImportSerializer::MetaExists(const std::filesystem::path &environmentFilesystemPath)
    {
        return std::filesystem::exists(GetMetaPath(environmentFilesystemPath));
    }

    std::string EnvironmentMapImportSerializer::ComputeSourceContentHash(
            const std::filesystem::path &environmentFilesystemPath)
    {
        std::error_code errorCode;
        const auto fileSize = std::filesystem::file_size(environmentFilesystemPath, errorCode);
        if (errorCode)
            return {};

        const auto writeTime = std::filesystem::last_write_time(environmentFilesystemPath, errorCode);
        if (errorCode)
            return {};

        const auto writeTimeCount = writeTime.time_since_epoch().count();
        std::ostringstream stream;
        stream << std::hex << fileSize << "-" << writeTimeCount;
        return stream.str();
    }

    bool EnvironmentMapImportSerializer::Deserialize(const std::filesystem::path &environmentFilesystemPath,
                                                     EnvironmentImportSettings &outSettings)
    {
        const std::filesystem::path metaPath = GetMetaPath(environmentFilesystemPath);
        if (!std::filesystem::exists(metaPath))
            return false;

        try
        {
            YAML::Node data = YAML::LoadFile(metaPath.string());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "EnvironmentImport")
                return false;

            if (data["CubemapResolution"])
                outSettings.CubemapResolution = data["CubemapResolution"].as<uint32_t>();
            if (data["IrradianceResolution"])
                outSettings.IrradianceResolution = data["IrradianceResolution"].as<uint32_t>();
            if (data["PrefilterResolution"])
                outSettings.PrefilterResolution = data["PrefilterResolution"].as<uint32_t>();
            if (data["PrefilterMipCount"])
                outSettings.PrefilterMipCount = data["PrefilterMipCount"].as<uint32_t>();
            if (data["BrdfLookupSize"])
                outSettings.BrdfLookupSize = data["BrdfLookupSize"].as<uint32_t>();
            if (data["SourceContentHash"])
                outSettings.SourceContentHash = data["SourceContentHash"].as<std::string>();
            return true;
        }
        catch (const std::exception &exception)
        {
            HIMII_CORE_ERROR("Failed to read environment meta {0}: {1}", metaPath.string(), exception.what());
            return false;
        }
    }

    bool EnvironmentMapImportSerializer::Serialize(const std::filesystem::path &environmentFilesystemPath,
                                                   const EnvironmentImportSettings &settings)
    {
        const std::filesystem::path metaPath = GetMetaPath(environmentFilesystemPath);
        try
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << "AssetType" << YAML::Value << "EnvironmentImport";
            out << YAML::Key << "Version" << YAML::Value << 1;
            out << YAML::Key << "CubemapResolution" << YAML::Value << settings.CubemapResolution;
            out << YAML::Key << "IrradianceResolution" << YAML::Value << settings.IrradianceResolution;
            out << YAML::Key << "PrefilterResolution" << YAML::Value << settings.PrefilterResolution;
            out << YAML::Key << "PrefilterMipCount" << YAML::Value << settings.PrefilterMipCount;
            out << YAML::Key << "BrdfLookupSize" << YAML::Value << settings.BrdfLookupSize;
            out << YAML::Key << "SourceContentHash" << YAML::Value << settings.SourceContentHash;
            out << YAML::EndMap;

            std::ofstream file(metaPath);
            if (!file.is_open())
                return false;
            file << out.c_str();
            return true;
        }
        catch (const std::exception &exception)
        {
            HIMII_CORE_ERROR("Failed to write environment meta {0}: {1}", metaPath.string(), exception.what());
            return false;
        }
    }

    void EnvironmentMapImportSerializer::EnsureDefaultMeta(const std::filesystem::path &environmentFilesystemPath)
    {
        EnvironmentImportSettings settings;
        if (Deserialize(environmentFilesystemPath, settings))
        {
            const std::string currentHash = ComputeSourceContentHash(environmentFilesystemPath);
            if (settings.SourceContentHash != currentHash)
            {
                settings.SourceContentHash = currentHash;
                Serialize(environmentFilesystemPath, settings);
            }
            return;
        }

        settings.SourceContentHash = ComputeSourceContentHash(environmentFilesystemPath);
        Serialize(environmentFilesystemPath, settings);
    }
}
