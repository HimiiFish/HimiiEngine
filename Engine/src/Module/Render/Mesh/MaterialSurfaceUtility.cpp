#include "Hepch.h"
#include "Module/Render/Mesh/MaterialSurfaceUtility.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Module/Render/Shader/ShaderCompilationService.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Resource/AssetManager.h"

namespace Himii
{
    namespace
    {
        Ref<Texture2D> ResolveTextureParameter(AssetManager *assetManager, const MaterialAsset &materialAsset,
                                               const ShaderAsset &shaderAsset, const char *parameterName)
        {
            const ShaderPropertyDefinition *textureDefinition =
                    shaderAsset.FindPropertyDefinition(parameterName);
            if (!textureDefinition || !assetManager)
                return nullptr;

            const MaterialParameterValue textureValue =
                    ResolveMaterialParameterValue(materialAsset, *textureDefinition);
            if (textureValue.TextureHandle == 0)
                return nullptr;

            Ref<Asset> textureBase = assetManager->GetAsset(textureValue.TextureHandle);
            if (!textureBase || textureBase->GetType() != AssetType::Texture2D)
                return nullptr;

            return std::static_pointer_cast<Texture2D>(textureBase);
        }

        AssetHandle ResolveTextureHandleParameter(const MaterialAsset &materialAsset,
                                                  const ShaderAsset &shaderAsset, const char *parameterName)
        {
            const ShaderPropertyDefinition *textureDefinition =
                    shaderAsset.FindPropertyDefinition(parameterName);
            if (!textureDefinition)
                return 0;

            return ResolveMaterialParameterValue(materialAsset, *textureDefinition).TextureHandle;
        }
    }

    ResolvedMaterialSurface GetEngineDefaultLitSurface()
    {
        ResolvedMaterialSurface surface;
        surface.AlbedoColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        surface.Metallic = 0.0f;
        surface.Roughness = 0.5f;
        surface.UsesLitPipeline = true;
        Ref<ShaderAsset> defaultShaderAsset =
                BuiltinShaderRegistry::GetBuiltinShaderAsset(BuiltinShaderRegistry::GetDefaultLitShaderHandle());
        if (defaultShaderAsset)
        {
            surface.ShaderAssetReference = defaultShaderAsset;
            surface.ShaderProgram = ShaderCompilationService::GetOrCompileShader(defaultShaderAsset);
        }
        return surface;
    }

    Ref<ShaderAsset> ResolveShaderAsset(AssetManager *assetManager, AssetHandle shaderHandle)
    {
        if (shaderHandle == 0)
            shaderHandle = BuiltinShaderRegistry::GetDefaultLitShaderHandle();

        if (BuiltinShaderHandles::IsBuiltinShaderHandle(shaderHandle))
            return BuiltinShaderRegistry::GetBuiltinShaderAsset(shaderHandle);

        if (!assetManager)
            return BuiltinShaderRegistry::GetBuiltinShaderAsset(BuiltinShaderRegistry::GetDefaultLitShaderHandle());

        Ref<Asset> shaderBase = assetManager->GetAsset(shaderHandle);
        if (!shaderBase || shaderBase->GetType() != AssetType::Shader)
            return BuiltinShaderRegistry::GetBuiltinShaderAsset(BuiltinShaderRegistry::GetDefaultLitShaderHandle());

        Ref<ShaderAsset> shaderAsset = std::static_pointer_cast<ShaderAsset>(shaderBase);
        ShaderCompilationService::GetOrCompileShader(shaderAsset);
        return shaderAsset;
    }

    MaterialParameterValue ResolveMaterialParameterValue(const MaterialAsset &materialAsset,
                                                         const ShaderPropertyDefinition &definition)
    {
        MaterialParameterValue resolvedValue;
        resolvedValue.Type = definition.Type;

        auto overrideIterator = materialAsset.ParameterOverrides.find(definition.Name);
        if (overrideIterator != materialAsset.ParameterOverrides.end())
            return overrideIterator->second;

        switch (definition.Type)
        {
            case ShaderPropertyType::Float:
                resolvedValue.FloatValue = definition.DefaultFloat;
                break;
            case ShaderPropertyType::Int:
                resolvedValue.IntValue = definition.DefaultInt;
                break;
            case ShaderPropertyType::Bool:
                resolvedValue.BoolValue = definition.DefaultBool;
                break;
            case ShaderPropertyType::Color:
                resolvedValue.ColorValue = definition.DefaultColor;
                break;
            case ShaderPropertyType::Vector2:
                resolvedValue.Vector2Value = definition.DefaultVector2;
                break;
            case ShaderPropertyType::Vector3:
                resolvedValue.Vector3Value = definition.DefaultVector3;
                break;
            case ShaderPropertyType::Vector4:
                resolvedValue.Vector4Value = definition.DefaultVector4;
                break;
            case ShaderPropertyType::Texture2D:
                resolvedValue.TextureHandle = 0;
                break;
        }

        return resolvedValue;
    }

    void ApplyMaterialParameterOverridesToShader(const MaterialAsset &materialAsset,
                                                   const Ref<ShaderAsset> &shaderAsset,
                                                   AssetManager *assetManager)
    {
        if (!shaderAsset || !shaderAsset->CompiledShader)
            return;

        Ref<Shader> shaderProgram = shaderAsset->CompiledShader;
        for (const ShaderPropertyDefinition &definition : shaderAsset->PropertyDefinitions)
        {
            const MaterialParameterValue parameterValue =
                    ResolveMaterialParameterValue(materialAsset, definition);

            switch (definition.Type)
            {
                case ShaderPropertyType::Float:
                    shaderProgram->SetFloat(definition.Name, parameterValue.FloatValue);
                    break;
                case ShaderPropertyType::Int:
                    shaderProgram->SetInt(definition.Name, parameterValue.IntValue);
                    break;
                case ShaderPropertyType::Bool:
                    shaderProgram->SetInt(definition.Name, parameterValue.BoolValue ? 1 : 0);
                    break;
                case ShaderPropertyType::Color:
                case ShaderPropertyType::Vector4:
                    shaderProgram->SetFloat4(definition.Name, parameterValue.ColorValue);
                    break;
                case ShaderPropertyType::Vector2:
                    shaderProgram->SetFloat2(definition.Name, parameterValue.Vector2Value);
                    break;
                case ShaderPropertyType::Vector3:
                    shaderProgram->SetFloat3(definition.Name, parameterValue.Vector3Value);
                    break;
                case ShaderPropertyType::Texture2D:
                    if (parameterValue.TextureHandle != 0 && assetManager)
                    {
                        Ref<Asset> textureBase = assetManager->GetAsset(parameterValue.TextureHandle);
                        if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                        {
                            Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(textureBase);
                            texture->Bind(definition.TextureBinding >= 0 ? definition.TextureBinding : 0);
                            shaderProgram->SetInt(definition.Name,
                                                  definition.TextureBinding >= 0 ? definition.TextureBinding : 0);
                        }
                    }
                    break;
            }
        }
    }

    ResolvedMaterialSurface ResolveMaterialSurface(AssetManager *assetManager, AssetHandle materialHandle)
    {
        if (materialHandle == 0 || !assetManager)
            return GetEngineDefaultLitSurface();

        Ref<Asset> materialBase = assetManager->GetAsset(materialHandle);
        if (!materialBase || materialBase->GetType() != AssetType::Material)
            return GetEngineDefaultLitSurface();

        Ref<MaterialAsset> materialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);
        Ref<ShaderAsset> shaderAsset = ResolveShaderAsset(assetManager, materialAsset->ShaderHandle);
        if (!shaderAsset)
            return GetEngineDefaultLitSurface();

        ResolvedMaterialSurface surface;
        surface.ShaderAssetReference = shaderAsset;
        surface.ShaderProgram = ShaderCompilationService::GetOrCompileShader(shaderAsset);
        surface.UsesLitPipeline = shaderAsset->PipelineType == ShaderPipelineType::SpatialLit;

        if (const ShaderPropertyDefinition *albedoColorDefinition =
                    shaderAsset->FindPropertyDefinition("u_AlbedoColor"))
        {
            surface.AlbedoColor =
                    ResolveMaterialParameterValue(*materialAsset, *albedoColorDefinition).ColorValue;
        }

        float metallicValue = 0.0f;
        if (const ShaderPropertyDefinition *metallicDefinition =
                    shaderAsset->FindPropertyDefinition("u_Metallic"))
        {
            metallicValue = ResolveMaterialParameterValue(*materialAsset, *metallicDefinition).FloatValue;
        }
        surface.Metallic = metallicValue;

        float roughnessValue = 0.5f;
        if (const ShaderPropertyDefinition *roughnessDefinition =
                    shaderAsset->FindPropertyDefinition("u_Roughness"))
        {
            roughnessValue = ResolveMaterialParameterValue(*materialAsset, *roughnessDefinition).FloatValue;
        }
        surface.Roughness = roughnessValue;

        surface.AlbedoTexture =
                ResolveTextureParameter(assetManager, *materialAsset, *shaderAsset, "u_AlbedoTexture");
        surface.MetallicTexture =
                ResolveTextureParameter(assetManager, *materialAsset, *shaderAsset, "u_MetallicTexture");
        surface.RoughnessTexture =
                ResolveTextureParameter(assetManager, *materialAsset, *shaderAsset, "u_RoughnessTexture");
        surface.NormalTexture =
                ResolveTextureParameter(assetManager, *materialAsset, *shaderAsset, "u_NormalTexture");

        if (const ShaderPropertyDefinition *normalFlipDefinition =
                    shaderAsset->FindPropertyDefinition("u_NormalFlipGreen"))
        {
            surface.NormalFlipGreen =
                    ResolveMaterialParameterValue(*materialAsset, *normalFlipDefinition).BoolValue;
        }

        const AssetHandle metallicHandle =
                ResolveTextureHandleParameter(*materialAsset, *shaderAsset, "u_MetallicTexture");
        const AssetHandle roughnessHandle =
                ResolveTextureHandleParameter(*materialAsset, *shaderAsset, "u_RoughnessTexture");
        surface.SharedMetallicRoughnessTexture =
                metallicHandle != 0 && metallicHandle == roughnessHandle;

        return surface;
    }
}
