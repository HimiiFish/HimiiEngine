#pragma once

#include "Module/Render/Shader/ShaderPropertyTypes.h"
#include "Resource/Asset.h"
#include <unordered_map>

namespace Himii
{
    /// Material 实例：引用 Shader 模板并覆盖其声明参数。
    class MaterialAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::Material; }

        AssetHandle ShaderHandle = 0;
        std::unordered_map<std::string, MaterialParameterValue> ParameterOverrides;

        void ClearParameterOverrides();
        void SetFloatParameter(const std::string &name, float value);
        void SetIntParameter(const std::string &name, int value);
        void SetColorParameter(const std::string &name, const glm::vec4 &value);
        void SetVector2Parameter(const std::string &name, const glm::vec2 &value);
        void SetVector3Parameter(const std::string &name, const glm::vec3 &value);
        void SetVector4Parameter(const std::string &name, const glm::vec4 &value);
        void SetTextureParameter(const std::string &name, AssetHandle textureHandle,
                                 const std::string &textureRelativePath = {});

        bool TryGetFloatParameter(const std::string &name, float &outValue) const;
        bool TryGetIntParameter(const std::string &name, int &outValue) const;
        bool TryGetColorParameter(const std::string &name, glm::vec4 &outValue) const;
        bool TryGetVector2Parameter(const std::string &name, glm::vec2 &outValue) const;
        bool TryGetVector3Parameter(const std::string &name, glm::vec3 &outValue) const;
        bool TryGetVector4Parameter(const std::string &name, glm::vec4 &outValue) const;
        bool TryGetTextureParameter(const std::string &name, AssetHandle &outTextureHandle,
                                    std::string &outTextureRelativePath) const;
    };
}
