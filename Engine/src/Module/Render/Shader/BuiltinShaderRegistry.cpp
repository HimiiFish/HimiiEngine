#include "Hepch.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Module/Render/Shader/ShaderCompilationService.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    namespace
    {
        std::string LoadEngineShaderSource(const char *relativeShaderPath)
        {
            return FileSystem::ReadText(relativeShaderPath);
        }

        Ref<ShaderAsset> CreateBuiltinShaderAsset(AssetHandle handle, ShaderPipelineType pipelineType,
                                                  const char *relativeShaderPath,
                                                  const std::vector<ShaderPropertyDefinition> &propertyDefinitions)
        {
            Ref<ShaderAsset> shaderAsset = CreateRef<ShaderAsset>();
            shaderAsset->Handle = handle;
            shaderAsset->IsBuiltin = true;
            shaderAsset->PipelineType = pipelineType;
            shaderAsset->PropertyDefinitions = propertyDefinitions;
            shaderAsset->SourceFilePath = relativeShaderPath;
            shaderAsset->SourceCode = LoadEngineShaderSource(relativeShaderPath);
            if (shaderAsset->SourceCode.empty())
            {
                HIMII_CORE_ERROR("Failed to load builtin shader source: {0}", relativeShaderPath);
                return shaderAsset;
            }

            ShaderCompilationService::TryCompileShaderAsset(shaderAsset, shaderAsset->CompiledShader);
            return shaderAsset;
        }
    }

    bool BuiltinShaderHandles::IsBuiltinShaderHandle(AssetHandle handle)
    {
        return handle == MeshLit || handle == MeshUnlit;
    }

    std::vector<ShaderPropertyDefinition> BuiltinShaderRegistry::GetMeshLitPropertyDefinitions()
    {
        std::vector<ShaderPropertyDefinition> definitions;
        {
            ShaderPropertyDefinition albedoColorDefinition;
            albedoColorDefinition.Name = "u_AlbedoColor";
            albedoColorDefinition.DisplayName = "Albedo Color";
            albedoColorDefinition.Type = ShaderPropertyType::Color;
            albedoColorDefinition.DefaultColor = glm::vec4(1.0f);
            definitions.push_back(albedoColorDefinition);
        }
        {
            ShaderPropertyDefinition metallicDefinition;
            metallicDefinition.Name = "u_Metallic";
            metallicDefinition.DisplayName = "Metallic";
            metallicDefinition.Type = ShaderPropertyType::Float;
            metallicDefinition.DefaultFloat = 0.0f;
            definitions.push_back(metallicDefinition);
        }
        {
            ShaderPropertyDefinition roughnessDefinition;
            roughnessDefinition.Name = "u_Roughness";
            roughnessDefinition.DisplayName = "Roughness";
            roughnessDefinition.Type = ShaderPropertyType::Float;
            roughnessDefinition.DefaultFloat = 0.5f;
            definitions.push_back(roughnessDefinition);
        }
        {
            ShaderPropertyDefinition albedoTextureDefinition;
            albedoTextureDefinition.Name = "u_AlbedoTexture";
            albedoTextureDefinition.DisplayName = "Albedo Texture";
            albedoTextureDefinition.Type = ShaderPropertyType::Texture2D;
            albedoTextureDefinition.TextureBinding = 0;
            definitions.push_back(albedoTextureDefinition);
        }
        {
            ShaderPropertyDefinition metallicTextureDefinition;
            metallicTextureDefinition.Name = "u_MetallicTexture";
            metallicTextureDefinition.DisplayName = "Metallic Texture";
            metallicTextureDefinition.Type = ShaderPropertyType::Texture2D;
            metallicTextureDefinition.TextureBinding = 1;
            definitions.push_back(metallicTextureDefinition);
        }
        {
            ShaderPropertyDefinition roughnessTextureDefinition;
            roughnessTextureDefinition.Name = "u_RoughnessTexture";
            roughnessTextureDefinition.DisplayName = "Roughness Texture";
            roughnessTextureDefinition.Type = ShaderPropertyType::Texture2D;
            roughnessTextureDefinition.TextureBinding = 2;
            definitions.push_back(roughnessTextureDefinition);
        }
        {
            ShaderPropertyDefinition normalTextureDefinition;
            normalTextureDefinition.Name = "u_NormalTexture";
            normalTextureDefinition.DisplayName = "Normal Texture";
            normalTextureDefinition.Type = ShaderPropertyType::Texture2D;
            normalTextureDefinition.TextureBinding = 3;
            definitions.push_back(normalTextureDefinition);
        }
        {
            ShaderPropertyDefinition normalFlipGreenDefinition;
            normalFlipGreenDefinition.Name = "u_NormalFlipGreen";
            normalFlipGreenDefinition.DisplayName = "Flip Green (DirectX)";
            normalFlipGreenDefinition.Type = ShaderPropertyType::Bool;
            normalFlipGreenDefinition.DefaultBool = false;
            definitions.push_back(normalFlipGreenDefinition);
        }
        return definitions;
    }

    std::vector<ShaderPropertyDefinition> BuiltinShaderRegistry::GetMeshUnlitPropertyDefinitions()
    {
        std::vector<ShaderPropertyDefinition> definitions;
        {
            ShaderPropertyDefinition albedoColorDefinition;
            albedoColorDefinition.Name = "u_AlbedoColor";
            albedoColorDefinition.DisplayName = "Albedo Color";
            albedoColorDefinition.Type = ShaderPropertyType::Color;
            albedoColorDefinition.DefaultColor = glm::vec4(1.0f);
            definitions.push_back(albedoColorDefinition);
        }
        ShaderPropertyDefinition albedoTextureDefinition;
        albedoTextureDefinition.Name = "u_AlbedoTexture";
        albedoTextureDefinition.DisplayName = "Albedo Texture";
        albedoTextureDefinition.Type = ShaderPropertyType::Texture2D;
        albedoTextureDefinition.TextureBinding = 0;
        definitions.push_back(albedoTextureDefinition);
        return definitions;
    }

    Ref<ShaderAsset> BuiltinShaderRegistry::GetBuiltinShaderAsset(AssetHandle handle)
    {
        static Ref<ShaderAsset> meshLitShaderAsset;
        static Ref<ShaderAsset> meshUnlitShaderAsset;

        if (handle == BuiltinShaderHandles::MeshLit)
        {
            if (!meshLitShaderAsset)
            {
                meshLitShaderAsset = CreateBuiltinShaderAsset(
                        BuiltinShaderHandles::MeshLit, ShaderPipelineType::SpatialLit,
                        "assets/shaders/Renderer3D_MeshLit.glsl", GetMeshLitPropertyDefinitions());
            }
            return meshLitShaderAsset;
        }

        if (handle == BuiltinShaderHandles::MeshUnlit)
        {
            if (!meshUnlitShaderAsset)
            {
                meshUnlitShaderAsset = CreateBuiltinShaderAsset(
                        BuiltinShaderHandles::MeshUnlit, ShaderPipelineType::SpatialUnlit,
                        "assets/shaders/Renderer3D_MeshUnlit.glsl", GetMeshUnlitPropertyDefinitions());
            }
            return meshUnlitShaderAsset;
        }

        return nullptr;
    }

    AssetHandle BuiltinShaderRegistry::GetDefaultLitShaderHandle()
    {
        return BuiltinShaderHandles::MeshLit;
    }

    AssetHandle BuiltinShaderRegistry::GetDefaultUnlitShaderHandle()
    {
        return BuiltinShaderHandles::MeshUnlit;
    }

    void BuiltinShaderRegistry::ApplyMeshLitDefaults(MaterialAsset &materialAsset)
    {
        materialAsset.ShaderHandle = BuiltinShaderHandles::MeshLit;
        materialAsset.ClearParameterOverrides();
        materialAsset.SetColorParameter("u_AlbedoColor", glm::vec4(1.0f));
        materialAsset.SetFloatParameter("u_Metallic", 0.0f);
        materialAsset.SetFloatParameter("u_Roughness", 0.5f);
        materialAsset.SetBoolParameter("u_NormalFlipGreen", false);
    }

    void BuiltinShaderRegistry::ApplyMeshUnlitDefaults(MaterialAsset &materialAsset)
    {
        materialAsset.ShaderHandle = BuiltinShaderHandles::MeshUnlit;
        materialAsset.ClearParameterOverrides();
        materialAsset.SetColorParameter("u_AlbedoColor", glm::vec4(1.0f));
    }
}
