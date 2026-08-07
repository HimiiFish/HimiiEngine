#include "Hepch.h"
#include "Module/Render/Mesh/StaticMeshImporter.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Mesh/HmeshAssetSerializer.h"
#include "Module/Render/Mesh/MeshSourceGeometryLoader.h"
#include "Module/Render/Mesh/MeshCompanionImport.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Project/Project.h"
#include "EngineCore/Core/Log.h"

#include <algorithm>
#include <cctype>

namespace Himii
{
    namespace
    {
        std::string NormalizePathExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return extension;
        }
    }

    std::filesystem::path StaticMeshImporter::GetProductPathForSource(
            const std::filesystem::path &relativeSourcePath)
    {
        return relativeSourcePath.parent_path()
               / (relativeSourcePath.stem().string() + ".hmesh");
    }

    AssetHandle StaticMeshImporter::ImportFromSource(AssetManager &assetManager,
                                                     const std::filesystem::path &relativeSourcePath,
                                                     const StaticMeshImportSettings &importSettings)
    {
        const std::filesystem::path absoluteSourcePath =
                Project::GetAssetFileSystemPath(relativeSourcePath);
        if (!std::filesystem::exists(absoluteSourcePath))
        {
            HIMII_CORE_ERROR("Static mesh import failed, source missing: {0}",
                             absoluteSourcePath.string());
            return 0;
        }

        const std::string sourceExtension = NormalizePathExtension(relativeSourcePath);
        if (!IsStaticMeshSourceExtension(sourceExtension))
        {
            HIMII_CORE_ERROR("Not a static mesh source file: {0}", relativeSourcePath.generic_string());
            return 0;
        }

        const std::filesystem::path relativeHmeshPath = GetProductPathForSource(relativeSourcePath);
        const std::filesystem::path absoluteHmeshPath =
                Project::GetAssetFileSystemPath(relativeHmeshPath);

        MeshAsset meshGeometry;
        if (!LoadMeshGeometryFromSource(absoluteSourcePath, importSettings, meshGeometry))
        {
            HIMII_CORE_ERROR("Failed to load mesh geometry from source: {0}",
                             absoluteSourcePath.string());
            return 0;
        }

        if (meshGeometry.Vertices.empty() || meshGeometry.Indices.empty())
        {
            HIMII_CORE_ERROR("Static mesh import produced empty geometry: {0}",
                             absoluteSourcePath.string());
            return 0;
        }

        if (!HmeshAssetSerializer::Serialize(absoluteHmeshPath, meshGeometry))
            return 0;

        std::vector<AssetHandle> materialHandles;
        std::vector<std::string> materialSlotNames;
        if (importSettings.ImportMaterialsAndTextures)
        {
            MeshCompanionImportResult companionResult;
            if (!ImportMeshCompanionAssets(assetManager, relativeSourcePath, &companionResult))
            {
                HIMII_CORE_WARNING("Static mesh geometry imported but companion materials failed: {0}",
                                   relativeSourcePath.generic_string());
            }
            else
            {
                materialHandles = std::move(companionResult.MaterialHandles);
                materialSlotNames = std::move(companionResult.MaterialSlotNames);
            }
        }

        if (!MeshAssetSerializer::WriteStaticMeshMeta(absoluteHmeshPath, importSettings,
                                                      relativeSourcePath, materialHandles,
                                                      materialSlotNames))
            return 0;

        const AssetHandle hmeshHandle = assetManager.ImportAsset(relativeHmeshPath);
        assetManager.UnloadAsset(hmeshHandle);
        assetManager.SerializeAssetRegistry();

        HIMII_CORE_INFO("Static mesh import done: {0} -> {1}", relativeSourcePath.generic_string(),
                        relativeHmeshPath.generic_string());
        return hmeshHandle;
    }

    AssetHandle StaticMeshImporter::ReimportProduct(AssetManager &assetManager,
                                                    const std::filesystem::path &relativeHmeshPath,
                                                    const StaticMeshImportSettings *overrideSettings,
                                                    bool preserveExistingCompanionMaterials)
    {
        const std::filesystem::path absoluteHmeshPath =
                Project::GetAssetFileSystemPath(relativeHmeshPath);

        StaticMeshImportSettings importSettings;
        std::filesystem::path relativeSourcePath;
        std::vector<AssetHandle> existingMaterialHandles;
        std::vector<std::string> existingSlotNames;

        if (!MeshAssetSerializer::ReadStaticMeshMeta(absoluteHmeshPath, importSettings,
                                                     relativeSourcePath, existingMaterialHandles,
                                                     existingSlotNames))
        {
            HIMII_CORE_ERROR("Cannot reimport static mesh without meta: {0}",
                             relativeHmeshPath.generic_string());
            return 0;
        }

        if (relativeSourcePath.empty())
        {
            HIMII_CORE_ERROR("Static mesh meta missing SourceFile: {0}",
                             relativeHmeshPath.generic_string());
            return 0;
        }

        if (overrideSettings)
            importSettings = *overrideSettings;

        const std::filesystem::path absoluteSourcePath =
                Project::GetAssetFileSystemPath(relativeSourcePath);
        if (!std::filesystem::exists(absoluteSourcePath))
        {
            HIMII_CORE_ERROR("Static mesh reimport failed, source missing: {0}",
                             absoluteSourcePath.string());
            return 0;
        }

        MeshAsset meshGeometry;
        if (!LoadMeshGeometryFromSource(absoluteSourcePath, importSettings, meshGeometry))
            return 0;

        if (!HmeshAssetSerializer::Serialize(absoluteHmeshPath, meshGeometry))
            return 0;

        std::vector<AssetHandle> materialHandles;
        std::vector<std::string> materialSlotNames;
        if (importSettings.ImportMaterialsAndTextures)
        {
            if (preserveExistingCompanionMaterials)
            {
                materialHandles = existingMaterialHandles;
                materialSlotNames = existingSlotNames;
            }
            else
            {
                MeshCompanionImportResult companionResult;
                ImportMeshCompanionAssets(assetManager, relativeSourcePath, &companionResult);
                materialHandles = std::move(companionResult.MaterialHandles);
                materialSlotNames = std::move(companionResult.MaterialSlotNames);
            }
        }

        MeshAssetSerializer::WriteStaticMeshMeta(absoluteHmeshPath, importSettings, relativeSourcePath,
                                                 materialHandles, materialSlotNames);

        AssetHandle hmeshHandle = 0;
        for (const auto &[handle, metadata] : assetManager.GetAssetRegistry())
        {
            if (metadata.FilePath.generic_string() == relativeHmeshPath.generic_string())
            {
                hmeshHandle = handle;
                break;
            }
        }

        if (hmeshHandle == 0)
            hmeshHandle = assetManager.ImportAsset(relativeHmeshPath);
        else
            assetManager.UnloadAsset(hmeshHandle);

        assetManager.SerializeAssetRegistry();
        HIMII_CORE_INFO("Static mesh reimport done: {0}", relativeHmeshPath.generic_string());
        return hmeshHandle;
    }
}
