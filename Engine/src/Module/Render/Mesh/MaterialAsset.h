#pragma once

#include "Resource/Asset.h"
#include <glm/glm.hpp>

namespace Himii
{
    enum class MaterialShadingMode
    {
        Lit = 0,
        Unlit = 1
    };

    /// Lit 表面材质（可选显式 Unlit）；默认新建/导入为 Lit。
    class MaterialAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::Material; }

        MaterialShadingMode ShadingMode = MaterialShadingMode::Lit;
        glm::vec4 AlbedoColor{1.0f, 1.0f, 1.0f, 1.0f};
        AssetHandle AlbedoTextureHandle = 0;
        /// 相对 Assets 目录的贴图路径；用于重开工程时 Handle 与 Registry 不同步的回退解析。
        std::string AlbedoTextureRelativePath;
        float Specular = 0.5f;
        float Shininess = 32.0f;
    };
}
