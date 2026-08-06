#include "Hepch.h"
#include "Module/Render/Mesh/StaticMeshImportSettings.h"

#include <algorithm>
#include <cctype>

namespace Himii
{
    bool IsStaticMeshSourceExtension(const std::string &extensionLowercase)
    {
        return extensionLowercase == ".glb" || extensionLowercase == ".gltf"
               || extensionLowercase == ".fbx" || extensionLowercase == ".obj";
    }
}
