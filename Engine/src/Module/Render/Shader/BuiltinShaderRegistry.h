#pragma once

#include "Module/Render/Shader/ShaderPropertyTypes.h"
#include "Resource/Asset.h"
#include <vector>

namespace Himii
{
    class ShaderAsset;

    namespace BuiltinShaderHandles
    {
        inline const AssetHandle MeshLit = AssetHandle(0x484D534800000001ULL);
        inline const AssetHandle MeshUnlit = AssetHandle(0x484D534800000002ULL);

        bool IsBuiltinShaderHandle(AssetHandle handle);
    }

    class BuiltinShaderRegistry
    {
    public:
        static Ref<ShaderAsset> GetBuiltinShaderAsset(AssetHandle handle);
        static AssetHandle GetDefaultLitShaderHandle();
        static AssetHandle GetDefaultUnlitShaderHandle();
        static std::vector<ShaderPropertyDefinition> GetMeshLitPropertyDefinitions();
        static std::vector<ShaderPropertyDefinition> GetMeshUnlitPropertyDefinitions();
        static void ApplyMeshLitDefaults(class MaterialAsset &materialAsset);
        static void ApplyMeshUnlitDefaults(class MaterialAsset &materialAsset);
    };
}
