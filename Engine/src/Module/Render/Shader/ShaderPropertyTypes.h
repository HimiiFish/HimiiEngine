#pragma once

#include "Resource/Asset.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Himii
{
    enum class ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector2 = 3,
        Vector3 = 4,
        Vector4 = 5,
        Texture2D = 6
    };

    enum class ShaderPipelineType
    {
        SpatialLit = 0,
        SpatialUnlit = 1
    };

    struct ShaderPropertyDefinition
    {
        std::string Name;
        std::string DisplayName;
        ShaderPropertyType Type = ShaderPropertyType::Float;
        float DefaultFloat = 0.0f;
        int DefaultInt = 0;
        glm::vec4 DefaultColor{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec2 DefaultVector2{0.0f, 0.0f};
        glm::vec3 DefaultVector3{0.0f, 0.0f, 0.0f};
        glm::vec4 DefaultVector4{0.0f, 0.0f, 0.0f, 0.0f};
        int TextureBinding = -1;
    };

    struct MaterialParameterValue
    {
        ShaderPropertyType Type = ShaderPropertyType::Float;
        float FloatValue = 0.0f;
        int IntValue = 0;
        glm::vec4 ColorValue{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec2 Vector2Value{0.0f, 0.0f};
        glm::vec3 Vector3Value{0.0f, 0.0f, 0.0f};
        glm::vec4 Vector4Value{0.0f, 0.0f, 0.0f, 0.0f};
        AssetHandle TextureHandle = 0;
        std::string TextureRelativePath;
    };

    ShaderPropertyType ShaderPropertyTypeFromString(const std::string &typeName);
    std::string ShaderPropertyTypeToString(ShaderPropertyType type);
    ShaderPipelineType ShaderPipelineTypeFromString(const std::string &pipelineName);
    std::string ShaderPipelineTypeToString(ShaderPipelineType pipelineType);
}
