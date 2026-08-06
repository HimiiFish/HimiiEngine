#include "Hepch.h"
#include "Module/Render/Mesh/MeshCompanionImport.h"
#include "Module/Render/Mesh/MeshAssetSerializer.h"
#include "Module/Render/Mesh/GltfMeshImporter.h"
#include "Module/Render/Mesh/FbxMeshImporter.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Project/Project.h"

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

    bool MeshCompanionMetaExists(const std::filesystem::path &absoluteMeshOrSourcePath)
    {
        const std::string extension = NormalizePathExtension(absoluteMeshOrSourcePath);
        if (extension == ".hmesh")
            return std::filesystem::exists(MeshAssetSerializer::GetMeshMetaPath(absoluteMeshOrSourcePath));

        if (IsStaticMeshSourceExtension(extension))
        {
            const std::filesystem::path productPath =
                    absoluteMeshOrSourcePath.parent_path()
                    / (absoluteMeshOrSourcePath.stem().string() + ".hmesh");
            return std::filesystem::exists(MeshAssetSerializer::GetMeshMetaPath(productPath));
        }

        return std::filesystem::exists(MeshAssetSerializer::GetMeshMetaPath(absoluteMeshOrSourcePath));
    }

    bool ImportMeshCompanionAssets(AssetManager &assetManager,
                                   const std::filesystem::path &relativeMeshPath,
                                   MeshCompanionImportResult *outResult)
    {
        const std::string extension = NormalizePathExtension(relativeMeshPath);
        if (extension == ".glb" || extension == ".gltf")
            return GltfMeshImporter::ImportCompanionAssets(assetManager, relativeMeshPath, outResult);
        if (extension == ".fbx" || extension == ".obj")
            return FbxMeshImporter::ImportCompanionAssets(assetManager, relativeMeshPath, outResult);

        HIMII_CORE_WARNING("No mesh companion importer for extension '{0}' ({1})", extension,
                           relativeMeshPath.generic_string());
        return false;
    }
}
