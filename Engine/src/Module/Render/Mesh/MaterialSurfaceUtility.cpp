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
    ResolvedMaterialSurface GetEngineDefaultLitSurface()
    {
        ResolvedMaterialSurface surface;
        surface.AlbedoColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        surface.Specular = 0.5f;
        surface.Shininess = 32.0f;
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

        float specularValue = 0.5f;
        if (const ShaderPropertyDefinition *specularDefinition =
                    shaderAsset->FindPropertyDefinition("u_Specular"))
        {
            specularValue = ResolveMaterialParameterValue(*materialAsset, *specularDefinition).FloatValue;
        }
        surface.Specular = specularValue;

        float shininessValue = 32.0f;
        if (const ShaderPropertyDefinition *shininessDefinition =
                    shaderAsset->FindPropertyDefinition("u_Shininess"))
        {
            shininessValue = ResolveMaterialParameterValue(*materialAsset, *shininessDefinition).FloatValue;
        }
        surface.Shininess = shininessValue;

        if (const ShaderPropertyDefinition *albedoTextureDefinition =
                    shaderAsset->FindPropertyDefinition("u_AlbedoTexture"))
        {
            const MaterialParameterValue textureValue =
                    ResolveMaterialParameterValue(*materialAsset, *albedoTextureDefinition);
            if (textureValue.TextureHandle != 0)
            {
                Ref<Asset> textureBase = assetManager->GetAsset(textureValue.TextureHandle);
                if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                    surface.AlbedoTexture = std::static_pointer_cast<Texture2D>(textureBase);
            }
        }

        return surface;
    }
}
