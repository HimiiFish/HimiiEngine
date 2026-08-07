#include "Hepch.h"
#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    namespace
    {
        void WriteParameterValue(YAML::Emitter &emitter, const MaterialParameterValue &parameterValue)
        {
            switch (parameterValue.Type)
            {
                case ShaderPropertyType::Float:
                    emitter << parameterValue.FloatValue;
                    break;
                case ShaderPropertyType::Int:
                    emitter << parameterValue.IntValue;
                    break;
                case ShaderPropertyType::Color:
                case ShaderPropertyType::Vector4:
                    emitter << YAML::Flow << YAML::BeginSeq << parameterValue.ColorValue.x
                            << parameterValue.ColorValue.y << parameterValue.ColorValue.z
                            << parameterValue.ColorValue.w << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Vector2:
                    emitter << YAML::Flow << YAML::BeginSeq << parameterValue.Vector2Value.x
                            << parameterValue.Vector2Value.y << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Vector3:
                    emitter << YAML::Flow << YAML::BeginSeq << parameterValue.Vector3Value.x
                            << parameterValue.Vector3Value.y << parameterValue.Vector3Value.z << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Texture2D:
                    emitter << YAML::BeginMap;
                    emitter << YAML::Key << "Handle" << YAML::Value
                            << static_cast<uint64_t>(parameterValue.TextureHandle);
                    if (!parameterValue.TextureRelativePath.empty())
                        emitter << YAML::Key << "RelativePath" << YAML::Value
                                << parameterValue.TextureRelativePath;
                    emitter << YAML::EndMap;
                    break;
            }
        }

        MaterialParameterValue ReadParameterValue(const YAML::Node &parameterNode, ShaderPropertyType type)
        {
            MaterialParameterValue parameterValue;
            parameterValue.Type = type;
            switch (type)
            {
                case ShaderPropertyType::Float:
                    parameterValue.FloatValue = parameterNode.as<float>();
                    break;
                case ShaderPropertyType::Int:
                    parameterValue.IntValue = parameterNode.as<int>();
                    break;
                case ShaderPropertyType::Color:
                case ShaderPropertyType::Vector4:
                    if (parameterNode.IsSequence() && parameterNode.size() >= 4)
                    {
                        parameterValue.ColorValue = {parameterNode[0].as<float>(), parameterNode[1].as<float>(),
                                                     parameterNode[2].as<float>(), parameterNode[3].as<float>()};
                    }
                    break;
                case ShaderPropertyType::Vector2:
                    if (parameterNode.IsSequence() && parameterNode.size() >= 2)
                    {
                        parameterValue.Vector2Value = {parameterNode[0].as<float>(), parameterNode[1].as<float>()};
                    }
                    break;
                case ShaderPropertyType::Vector3:
                    if (parameterNode.IsSequence() && parameterNode.size() >= 3)
                    {
                        parameterValue.Vector3Value = {parameterNode[0].as<float>(), parameterNode[1].as<float>(),
                                                       parameterNode[2].as<float>()};
                    }
                    break;
                case ShaderPropertyType::Texture2D:
                    if (parameterNode["Handle"])
                        parameterValue.TextureHandle = parameterNode["Handle"].as<uint64_t>();
                    if (parameterNode["RelativePath"])
                        parameterValue.TextureRelativePath = parameterNode["RelativePath"].as<std::string>();
                    break;
            }
            return parameterValue;
        }

        glm::vec4 ReadLegacyVector4(const YAML::Node &node, const char *key, const glm::vec4 &defaultValue)
        {
            if (!node[key] || !node[key].IsSequence() || node[key].size() < 4)
                return defaultValue;
            return {node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>(),
                    node[key][3].as<float>()};
        }

        void MigrateLegacyMaterialFields(const YAML::Node &data, Ref<MaterialAsset> &asset)
        {
            if (data["ShaderHandle"])
                return;

            const int shadingModeValue = data["ShadingMode"] ? data["ShadingMode"].as<int>() : 0;
            if (shadingModeValue == 1)
                BuiltinShaderRegistry::ApplyMeshUnlitDefaults(*asset);
            else
                BuiltinShaderRegistry::ApplyMeshLitDefaults(*asset);

            asset->SetColorParameter("u_AlbedoColor",
                                     ReadLegacyVector4(data, "AlbedoColor", glm::vec4(1.0f)));
            if (data["AlbedoTextureHandle"])
            {
                asset->SetTextureParameter("u_AlbedoTexture", data["AlbedoTextureHandle"].as<uint64_t>(),
                                           data["AlbedoTextureRelativePath"]
                                                   ? data["AlbedoTextureRelativePath"].as<std::string>()
                                                   : std::string{});
            }
            if (data["Specular"])
                asset->SetFloatParameter("u_Specular", data["Specular"].as<float>());
            if (data["Shininess"])
                asset->SetFloatParameter("u_Shininess", data["Shininess"].as<float>());
        }
    }

    void MaterialAssetSerializer::Serialize(const std::filesystem::path &filepath, const Ref<MaterialAsset> &asset)
    {
        if (!asset)
            return;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetType" << YAML::Value << "Material";
        emitter << YAML::Key << "Handle" << YAML::Value << static_cast<uint64_t>(asset->Handle);
        emitter << YAML::Key << "ShaderHandle" << YAML::Value << static_cast<uint64_t>(asset->ShaderHandle);
        emitter << YAML::Key << "Parameters" << YAML::Value << YAML::BeginMap;
        for (const auto &[parameterName, parameterValue] : asset->ParameterOverrides)
        {
            emitter << YAML::Key << parameterName << YAML::Value;
            WriteParameterValue(emitter, parameterValue);
        }
        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        std::ofstream outputFile(filepath);
        outputFile << emitter.c_str();
    }

    Ref<MaterialAsset> MaterialAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream inputStream(filepath);
            if (!inputStream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open MaterialAsset file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream stringStream;
            stringStream << inputStream.rdbuf();
            YAML::Node data = YAML::Load(stringStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "Material")
            {
                HIMII_CORE_ERROR("Invalid MaterialAsset: {0}", filepath.string());
                return nullptr;
            }

            Ref<MaterialAsset> asset = CreateRef<MaterialAsset>();
            if (data["Handle"])
                asset->Handle = data["Handle"].as<uint64_t>();

            MigrateLegacyMaterialFields(data, asset);

            if (data["ShaderHandle"])
                asset->ShaderHandle = data["ShaderHandle"].as<uint64_t>();
            else if (asset->ShaderHandle == 0)
                asset->ShaderHandle = BuiltinShaderRegistry::GetDefaultLitShaderHandle();

            if (data["Parameters"] && data["Parameters"].IsMap())
            {
                for (const auto &parameterEntry : data["Parameters"])
                {
                    const std::string parameterName = parameterEntry.first.as<std::string>();
                    ShaderPropertyType parameterType = ShaderPropertyType::Float;
                    if (parameterEntry.second.IsMap() && parameterEntry.second["Handle"])
                        parameterType = ShaderPropertyType::Texture2D;
                    else if (parameterEntry.second.IsSequence())
                    {
                        if (parameterEntry.second.size() == 2)
                            parameterType = ShaderPropertyType::Vector2;
                        else if (parameterEntry.second.size() == 3)
                            parameterType = ShaderPropertyType::Vector3;
                        else
                            parameterType = ShaderPropertyType::Color;
                    }
                    else if (parameterEntry.second.IsScalar())
                    {
                        if (parameterEntry.second.Tag() == "!")
                            parameterType = ShaderPropertyType::Int;
                        else
                            parameterType = ShaderPropertyType::Float;
                    }

                    asset->ParameterOverrides[parameterName] =
                            ReadParameterValue(parameterEntry.second, parameterType);
                }
            }

            return asset;
        }
        catch (const YAML::Exception &exception)
        {
            HIMII_CORE_ERROR("Failed to deserialize MaterialAsset '{0}': {1}", filepath.string(),
                             exception.what());
            return nullptr;
        }
    }

    Ref<MaterialAsset> MaterialAssetSerializer::CreateDefaultMaterialInstance(AssetHandle shaderHandle)
    {
        Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>();
        materialAsset->ShaderHandle = shaderHandle;
        if (shaderHandle == BuiltinShaderHandles::MeshUnlit)
            BuiltinShaderRegistry::ApplyMeshUnlitDefaults(*materialAsset);
        else
            BuiltinShaderRegistry::ApplyMeshLitDefaults(*materialAsset);
        return materialAsset;
    }
}
