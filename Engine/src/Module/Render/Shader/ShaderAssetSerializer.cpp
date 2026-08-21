#include "Hepch.h"
#include "Module/Render/Shader/ShaderAssetSerializer.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    namespace
    {
        glm::vec4 ReadDefaultColor(const YAML::Node &propertyNode)
        {
            if (!propertyNode["Default"] || !propertyNode["Default"].IsSequence()
                || propertyNode["Default"].size() < 4)
            {
                return glm::vec4(1.0f);
            }
            return {propertyNode["Default"][0].as<float>(), propertyNode["Default"][1].as<float>(),
                    propertyNode["Default"][2].as<float>(), propertyNode["Default"][3].as<float>()};
        }

        glm::vec2 ReadDefaultVector2(const YAML::Node &propertyNode)
        {
            if (!propertyNode["Default"] || !propertyNode["Default"].IsSequence()
                || propertyNode["Default"].size() < 2)
            {
                return glm::vec2(0.0f);
            }
            return {propertyNode["Default"][0].as<float>(), propertyNode["Default"][1].as<float>()};
        }

        glm::vec3 ReadDefaultVector3(const YAML::Node &propertyNode)
        {
            if (!propertyNode["Default"] || !propertyNode["Default"].IsSequence()
                || propertyNode["Default"].size() < 3)
            {
                return glm::vec3(0.0f);
            }
            return {propertyNode["Default"][0].as<float>(), propertyNode["Default"][1].as<float>(),
                    propertyNode["Default"][2].as<float>()};
        }

        glm::vec4 ReadDefaultVector4(const YAML::Node &propertyNode)
        {
            return ReadDefaultColor(propertyNode);
        }

        ShaderPropertyDefinition ReadPropertyDefinition(const YAML::Node &propertyNode)
        {
            ShaderPropertyDefinition definition;
            if (propertyNode["Name"])
                definition.Name = propertyNode["Name"].as<std::string>();
            if (propertyNode["DisplayName"])
                definition.DisplayName = propertyNode["DisplayName"].as<std::string>();
            else
                definition.DisplayName = definition.Name;

            const std::string typeName = propertyNode["Type"] ? propertyNode["Type"].as<std::string>() : "Float";
            definition.Type = ShaderPropertyTypeFromString(typeName);

            switch (definition.Type)
            {
                case ShaderPropertyType::Float:
                    definition.DefaultFloat = propertyNode["Default"] ? propertyNode["Default"].as<float>() : 0.0f;
                    break;
                case ShaderPropertyType::Int:
                    definition.DefaultInt = propertyNode["Default"] ? propertyNode["Default"].as<int>() : 0;
                    break;
                case ShaderPropertyType::Bool:
                    definition.DefaultBool = propertyNode["Default"] ? propertyNode["Default"].as<bool>() : false;
                    break;
                case ShaderPropertyType::Color:
                    definition.DefaultColor = ReadDefaultColor(propertyNode);
                    break;
                case ShaderPropertyType::Vector2:
                    definition.DefaultVector2 = ReadDefaultVector2(propertyNode);
                    break;
                case ShaderPropertyType::Vector3:
                    definition.DefaultVector3 = ReadDefaultVector3(propertyNode);
                    break;
                case ShaderPropertyType::Vector4:
                    definition.DefaultVector4 = ReadDefaultVector4(propertyNode);
                    break;
                case ShaderPropertyType::Texture2D:
                    definition.TextureBinding = propertyNode["Binding"] ? propertyNode["Binding"].as<int>() : 0;
                    break;
            }

            return definition;
        }

        bool SplitHeaderAndSource(const std::string &fileContents, std::string &outHeader, std::string &outSource)
        {
            const std::string separatorMarker = std::string("\n") + ShaderAssetSerializer::SourceSeparator;
            const size_t separatorPosition = fileContents.find(separatorMarker);
            if (separatorPosition == std::string::npos)
                return false;

            outHeader = fileContents.substr(0, separatorPosition);
            size_t sourceStart = separatorPosition + separatorMarker.size();
            if (sourceStart < fileContents.size() && fileContents[sourceStart] == '\n')
                sourceStart++;
            outSource = fileContents.substr(sourceStart);
            return true;
        }
    }

    void ShaderAssetSerializer::Serialize(const std::filesystem::path &filepath, const Ref<ShaderAsset> &asset)
    {
        if (!asset)
            return;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetType" << YAML::Value << "Shader";
        emitter << YAML::Key << "Handle" << YAML::Value << static_cast<uint64_t>(asset->Handle);
        emitter << YAML::Key << "Pipeline" << YAML::Value << ShaderPipelineTypeToString(asset->PipelineType);
        emitter << YAML::Key << "Properties" << YAML::Value << YAML::BeginSeq;
        for (const ShaderPropertyDefinition &definition : asset->PropertyDefinitions)
        {
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Name" << YAML::Value << definition.Name;
            emitter << YAML::Key << "DisplayName" << YAML::Value << definition.DisplayName;
            emitter << YAML::Key << "Type" << YAML::Value << ShaderPropertyTypeToString(definition.Type);
            switch (definition.Type)
            {
                case ShaderPropertyType::Float:
                    emitter << YAML::Key << "Default" << YAML::Value << definition.DefaultFloat;
                    break;
                case ShaderPropertyType::Int:
                    emitter << YAML::Key << "Default" << YAML::Value << definition.DefaultInt;
                    break;
                case ShaderPropertyType::Bool:
                    emitter << YAML::Key << "Default" << YAML::Value << definition.DefaultBool;
                    break;
                case ShaderPropertyType::Color:
                    emitter << YAML::Key << "Default" << YAML::Value << YAML::Flow << YAML::BeginSeq
                            << definition.DefaultColor.x << definition.DefaultColor.y << definition.DefaultColor.z
                            << definition.DefaultColor.w << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Vector2:
                    emitter << YAML::Key << "Default" << YAML::Value << YAML::Flow << YAML::BeginSeq
                            << definition.DefaultVector2.x << definition.DefaultVector2.y << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Vector3:
                    emitter << YAML::Key << "Default" << YAML::Value << YAML::Flow << YAML::BeginSeq
                            << definition.DefaultVector3.x << definition.DefaultVector3.y
                            << definition.DefaultVector3.z << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Vector4:
                    emitter << YAML::Key << "Default" << YAML::Value << YAML::Flow << YAML::BeginSeq
                            << definition.DefaultVector4.x << definition.DefaultVector4.y
                            << definition.DefaultVector4.z << definition.DefaultVector4.w << YAML::EndSeq;
                    break;
                case ShaderPropertyType::Texture2D:
                    emitter << YAML::Key << "Binding" << YAML::Value << definition.TextureBinding;
                    break;
            }
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;

        std::ofstream outputFile(filepath);
        outputFile << emitter.c_str() << "\n" << SourceSeparator << "\n" << asset->SourceCode;
    }

    Ref<ShaderAsset> ShaderAssetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream inputStream(filepath);
            if (!inputStream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open ShaderAsset file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream stringStream;
            stringStream << inputStream.rdbuf();
            const std::string fileContents = stringStream.str();

            std::string headerText;
            std::string sourceText;
            if (!SplitHeaderAndSource(fileContents, headerText, sourceText))
            {
                HIMII_CORE_ERROR("Shader asset missing source separator '---': {0}", filepath.string());
                return nullptr;
            }

            YAML::Node data = YAML::Load(headerText);
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "Shader")
            {
                HIMII_CORE_ERROR("Invalid ShaderAsset header: {0}", filepath.string());
                return nullptr;
            }

            Ref<ShaderAsset> asset = CreateRef<ShaderAsset>();
            asset->SourceFilePath = filepath;
            asset->SourceCode = sourceText;
            if (data["Handle"])
                asset->Handle = data["Handle"].as<uint64_t>();
            if (data["Pipeline"])
                asset->PipelineType = ShaderPipelineTypeFromString(data["Pipeline"].as<std::string>());

            if (data["Properties"] && data["Properties"].IsSequence())
            {
                for (const auto &propertyNode : data["Properties"])
                    asset->PropertyDefinitions.push_back(ReadPropertyDefinition(propertyNode));
            }

            return asset;
        }
        catch (const YAML::Exception &exception)
        {
            HIMII_CORE_ERROR("Failed to deserialize ShaderAsset '{0}': {1}", filepath.string(), exception.what());
            return nullptr;
        }
    }

    std::string ShaderAssetSerializer::BuildDefaultSpatialLitTemplate()
    {
        return R"(#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureCoordinate;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshLitUniforms
{
    mat4 u_Transform;
    vec4 u_AlbedoColor;
    float u_Metallic;
    float u_Roughness;
    int u_UseAlbedoTexture;
    int u_UseMetallicTexture;
    int u_UseRoughnessTexture;
    int u_SharedMetallicRoughnessTexture;
    int u_EntityID;
};

layout(location = 0) out vec2 v_TextureCoordinate;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec3 v_WorldPosition;

void main()
{
    v_TextureCoordinate = a_TextureCoordinate;
    vec4 worldPosition = u_Transform * vec4(a_Position, 1.0);
    v_WorldPosition = worldPosition.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(u_Transform)));
    v_Normal = normalize(normalMatrix * a_Normal);
    gl_Position = u_ViewProjection * worldPosition;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

layout(location = 0) in vec2 v_TextureCoordinate;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec3 v_WorldPosition;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    vec4 u_CameraPosition;
};

layout(std140, binding = 3) uniform MeshLitUniforms
{
    mat4 u_Transform;
    vec4 u_AlbedoColor;
    float u_Metallic;
    float u_Roughness;
    int u_UseAlbedoTexture;
    int u_UseMetallicTexture;
    int u_UseRoughnessTexture;
    int u_SharedMetallicRoughnessTexture;
    int u_EntityID;
};

layout(std140, binding = 4) uniform SceneLighting
{
    vec4 u_DirectionalLightDirectionIntensity;
    vec4 u_DirectionalLightColor;
    vec4 u_AmbientColorIntensity;
    mat4 u_LightViewProjection[4];
    vec4 u_ShadowParameters;
    vec4 u_CascadeSplitDistances;
    vec4 u_ShadowTexelWorldSize;
    vec4 u_ShadowViewerForwardAndOverlap;
    vec4 u_PointLightCount;
    vec4 u_PointLightPositionRange[8];
    vec4 u_PointLightColorIntensity[8];
    vec4 u_ImageBasedLightingParameters;
};

layout(binding = 0) uniform sampler2D u_AlbedoTexture;
layout(binding = 31) uniform sampler2DShadow u_ShadowMap;

vec2 ComputeCascadeAtlasCoordinates(vec2 projectedCoordinates, int cascadeIndex)
{
    float paddingUv = u_ShadowParameters.z * 2.0;
    float innerSize = 0.5 - 2.0 * paddingUv;
    vec2 tileOrigin = vec2(float(cascadeIndex % 2), float(cascadeIndex / 2)) * 0.5;
    return tileOrigin + vec2(paddingUv) + projectedCoordinates * innerSize;
}

int SelectDirectionalShadowCascade(float viewDistance)
{
    if (viewDistance < u_CascadeSplitDistances.x)
        return 0;
    if (viewDistance < u_CascadeSplitDistances.y)
        return 1;
    if (viewDistance < u_CascadeSplitDistances.z)
        return 2;
    return 3;
}

float SampleDirectionalShadowCascade(vec3 worldPosition, vec3 surfaceNormal, float surfaceSlope, int cascadeIndex)
{
    float texelWorldSize = u_ShadowTexelWorldSize.x;
    mat4 lightViewProjection = u_LightViewProjection[0];
    if (cascadeIndex == 1)
    {
        texelWorldSize = u_ShadowTexelWorldSize.y;
        lightViewProjection = u_LightViewProjection[1];
    }
    else if (cascadeIndex == 2)
    {
        texelWorldSize = u_ShadowTexelWorldSize.z;
        lightViewProjection = u_LightViewProjection[2];
    }
    else if (cascadeIndex >= 3)
    {
        texelWorldSize = u_ShadowTexelWorldSize.w;
        lightViewProjection = u_LightViewProjection[3];
    }
    float normalOffsetWorld = texelWorldSize * 1.5 * (1.0 + surfaceSlope * 2.0);
    vec4 lightSpacePosition =
        lightViewProjection * vec4(worldPosition + surfaceNormal * normalOffsetWorld, 1.0);
    vec3 projectedCoordinates = lightSpacePosition.xyz / lightSpacePosition.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
    if (projectedCoordinates.z > 1.0
        || projectedCoordinates.x < 0.0 || projectedCoordinates.x > 1.0
        || projectedCoordinates.y < 0.0 || projectedCoordinates.y > 1.0)
        return 1.0;
    vec2 atlasCoordinates = ComputeCascadeAtlasCoordinates(projectedCoordinates.xy, cascadeIndex);
    float depthBias = u_ShadowParameters.y * (1.0 + surfaceSlope * 2.0);
    return texture(u_ShadowMap, vec3(atlasCoordinates, projectedCoordinates.z - depthBias));
}

float SampleDirectionalShadow(vec3 worldPosition, vec3 normal, vec3 lightDirectionTowardSurface)
{
    if (u_ShadowParameters.x < 0.5)
        return 1.0;
    vec3 surfaceNormal = normalize(normal);
    float surfaceSlope = clamp(1.0 - max(dot(surfaceNormal, lightDirectionTowardSurface), 0.0), 0.0, 1.0);
    vec3 viewerForward = normalize(u_ShadowViewerForwardAndOverlap.xyz);
    float viewDistance = dot(worldPosition - u_CameraPosition.xyz, viewerForward);
    if (viewDistance > u_CascadeSplitDistances.w)
        return 1.0;
    int cascadeIndex = SelectDirectionalShadowCascade(viewDistance);
    float currentSample = SampleDirectionalShadowCascade(worldPosition, surfaceNormal, surfaceSlope, cascadeIndex);
    if (cascadeIndex >= 3)
        return currentSample;
    float cascadeNear = cascadeIndex == 0 ? u_ShadowParameters.w
        : (cascadeIndex == 1 ? u_CascadeSplitDistances.x : u_CascadeSplitDistances.y);
    float cascadeFar = cascadeIndex == 0 ? u_CascadeSplitDistances.x
        : (cascadeIndex == 1 ? u_CascadeSplitDistances.y : u_CascadeSplitDistances.z);
    float blendWidth = max((cascadeFar - cascadeNear) * u_ShadowViewerForwardAndOverlap.w, 0.0001);
    float blendFactor = clamp((cascadeFar - viewDistance) / blendWidth, 0.0, 1.0);
    if (blendFactor >= 1.0)
        return currentSample;
    float nextSample = SampleDirectionalShadowCascade(worldPosition, surfaceNormal, surfaceSlope, cascadeIndex + 1);
    return mix(nextSample, currentSample, blendFactor);
}

void main()
{
    vec4 albedo = u_AlbedoColor;
    if (u_UseAlbedoTexture != 0)
        albedo *= texture(u_AlbedoTexture, v_TextureCoordinate);

    vec3 normal = normalize(v_Normal);
    vec3 lightDirection = normalize(-u_DirectionalLightDirectionIntensity.xyz);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 ambient = u_AmbientColorIntensity.rgb * u_AmbientColorIntensity.a;
    float shadowFactor = SampleDirectionalShadow(v_WorldPosition, normal, lightDirection);
    vec3 lit = ambient + u_DirectionalLightColor.rgb * diffuse * shadowFactor;
    o_Color = vec4(lit * albedo.rgb, albedo.a);
    o_EntityID = u_EntityID;
}
)";
    }
}
