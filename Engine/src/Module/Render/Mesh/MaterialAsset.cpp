#include "Hepch.h"
#include "Module/Render/Mesh/MaterialAsset.h"

namespace Himii
{
    void MaterialAsset::ClearParameterOverrides()
    {
        ParameterOverrides.clear();
    }

    void MaterialAsset::SetFloatParameter(const std::string &name, float value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Float;
        parameterValue.FloatValue = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetIntParameter(const std::string &name, int value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Int;
        parameterValue.IntValue = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetBoolParameter(const std::string &name, bool value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Bool;
        parameterValue.BoolValue = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetColorParameter(const std::string &name, const glm::vec4 &value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Color;
        parameterValue.ColorValue = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetVector2Parameter(const std::string &name, const glm::vec2 &value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Vector2;
        parameterValue.Vector2Value = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetVector3Parameter(const std::string &name, const glm::vec3 &value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Vector3;
        parameterValue.Vector3Value = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetVector4Parameter(const std::string &name, const glm::vec4 &value)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Vector4;
        parameterValue.Vector4Value = value;
        ParameterOverrides[name] = parameterValue;
    }

    void MaterialAsset::SetTextureParameter(const std::string &name, AssetHandle textureHandle,
                                            const std::string &textureRelativePath)
    {
        MaterialParameterValue parameterValue;
        parameterValue.Type = ShaderPropertyType::Texture2D;
        parameterValue.TextureHandle = textureHandle;
        parameterValue.TextureRelativePath = textureRelativePath;
        ParameterOverrides[name] = parameterValue;
    }

    bool MaterialAsset::TryGetFloatParameter(const std::string &name, float &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Float)
            return false;
        outValue = iterator->second.FloatValue;
        return true;
    }

    bool MaterialAsset::TryGetIntParameter(const std::string &name, int &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Int)
            return false;
        outValue = iterator->second.IntValue;
        return true;
    }

    bool MaterialAsset::TryGetBoolParameter(const std::string &name, bool &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Bool)
            return false;
        outValue = iterator->second.BoolValue;
        return true;
    }

    bool MaterialAsset::TryGetColorParameter(const std::string &name, glm::vec4 &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Color)
            return false;
        outValue = iterator->second.ColorValue;
        return true;
    }

    bool MaterialAsset::TryGetVector2Parameter(const std::string &name, glm::vec2 &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Vector2)
            return false;
        outValue = iterator->second.Vector2Value;
        return true;
    }

    bool MaterialAsset::TryGetVector3Parameter(const std::string &name, glm::vec3 &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Vector3)
            return false;
        outValue = iterator->second.Vector3Value;
        return true;
    }

    bool MaterialAsset::TryGetVector4Parameter(const std::string &name, glm::vec4 &outValue) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Vector4)
            return false;
        outValue = iterator->second.Vector4Value;
        return true;
    }

    bool MaterialAsset::TryGetTextureParameter(const std::string &name, AssetHandle &outTextureHandle,
                                               std::string &outTextureRelativePath) const
    {
        auto iterator = ParameterOverrides.find(name);
        if (iterator == ParameterOverrides.end() || iterator->second.Type != ShaderPropertyType::Texture2D)
            return false;
        outTextureHandle = iterator->second.TextureHandle;
        outTextureRelativePath = iterator->second.TextureRelativePath;
        return true;
    }
}
