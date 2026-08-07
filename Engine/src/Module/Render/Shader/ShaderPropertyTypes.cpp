#include "Hepch.h"
#include "Module/Render/Shader/ShaderPropertyTypes.h"

namespace Himii
{
    ShaderPropertyType ShaderPropertyTypeFromString(const std::string &typeName)
    {
        if (typeName == "Float")
            return ShaderPropertyType::Float;
        if (typeName == "Int")
            return ShaderPropertyType::Int;
        if (typeName == "Color")
            return ShaderPropertyType::Color;
        if (typeName == "Vector2")
            return ShaderPropertyType::Vector2;
        if (typeName == "Vector3")
            return ShaderPropertyType::Vector3;
        if (typeName == "Vector4")
            return ShaderPropertyType::Vector4;
        if (typeName == "Texture2D")
            return ShaderPropertyType::Texture2D;
        return ShaderPropertyType::Float;
    }

    std::string ShaderPropertyTypeToString(ShaderPropertyType type)
    {
        switch (type)
        {
            case ShaderPropertyType::Float:
                return "Float";
            case ShaderPropertyType::Int:
                return "Int";
            case ShaderPropertyType::Color:
                return "Color";
            case ShaderPropertyType::Vector2:
                return "Vector2";
            case ShaderPropertyType::Vector3:
                return "Vector3";
            case ShaderPropertyType::Vector4:
                return "Vector4";
            case ShaderPropertyType::Texture2D:
                return "Texture2D";
        }
        return "Float";
    }

    ShaderPipelineType ShaderPipelineTypeFromString(const std::string &pipelineName)
    {
        if (pipelineName == "SpatialUnlit")
            return ShaderPipelineType::SpatialUnlit;
        return ShaderPipelineType::SpatialLit;
    }

    std::string ShaderPipelineTypeToString(ShaderPipelineType pipelineType)
    {
        switch (pipelineType)
        {
            case ShaderPipelineType::SpatialUnlit:
                return "SpatialUnlit";
            case ShaderPipelineType::SpatialLit:
            default:
                return "SpatialLit";
        }
    }
}
