#include "Hepch.h"
#include "Module/Render/Mesh/MeshSourceGeometryLoader.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"
#include "Module/Render/Mesh/GltfMeshGeometryLoader.h"
#include "Module/Render/Mesh/FbxMeshGeometryLoader.h"
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

        void ApplyUniformScale(MeshAsset &meshAsset, float uniformScale)
        {
            if (uniformScale == 1.0f)
                return;

            for (MeshVertex &vertex : meshAsset.Vertices)
                vertex.Position *= uniformScale;
        }
    }

    bool LoadMeshGeometryFromSource(const std::filesystem::path &absoluteSourcePath,
                                    const StaticMeshImportSettings &importSettings,
                                    MeshAsset &outMeshAsset)
    {
        (void)importSettings.CombineMeshes;

        outMeshAsset.Vertices.clear();
        outMeshAsset.Indices.clear();
        outMeshAsset.Submeshes.clear();

        const std::string extension = NormalizePathExtension(absoluteSourcePath);
        bool geometryLoaded = false;

        if (extension == ".glb" || extension == ".gltf")
            geometryLoaded = LoadGltfMeshGeometry(absoluteSourcePath, outMeshAsset);
        else if (extension == ".fbx" || extension == ".obj")
            geometryLoaded = LoadFbxMeshGeometry(absoluteSourcePath, outMeshAsset);
        else
        {
            HIMII_CORE_ERROR("Unsupported static mesh source extension '{0}': {1}", extension,
                             absoluteSourcePath.string());
            return false;
        }

        if (!geometryLoaded)
            return false;

        ApplyUniformScale(outMeshAsset, importSettings.UniformScale);
        return true;
    }
}
