#pragma once

#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Shader/ShaderPropertyTypes.h"
#include "Module/Render/RenderCore/Shader.h"
#include "EngineCore/Core/Core.h"
#include <glm/glm.hpp>

namespace Himii
{
    class AssetManager;
    class ShaderAsset;
    class Texture2D;

    struct ResolvedMaterialSurface
    {
        glm::vec4 AlbedoColor{0.5f, 0.5f, 0.5f, 1.0f};
        float Specular = 0.5f;
        float Shininess = 32.0f;
        bool UsesLitPipeline = true;
        Ref<Texture2D> AlbedoTexture;
        Ref<Shader> ShaderProgram;
        Ref<ShaderAsset> ShaderAssetReference;
    };

    ResolvedMaterialSurface GetEngineDefaultLitSurface();
    Ref<ShaderAsset> ResolveShaderAsset(AssetManager *assetManager, AssetHandle shaderHandle);
    ResolvedMaterialSurface ResolveMaterialSurface(AssetManager *assetManager, AssetHandle materialHandle);
    MaterialParameterValue ResolveMaterialParameterValue(const MaterialAsset &materialAsset,
                                                         const ShaderPropertyDefinition &definition);
    void ApplyMaterialParameterOverridesToShader(const MaterialAsset &materialAsset,
                                                   const Ref<ShaderAsset> &shaderAsset,
                                                   AssetManager *assetManager);
}
