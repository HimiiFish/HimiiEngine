#pragma once

#include "Resource/Asset.h"
#include <glm/glm.hpp>

namespace Himii
{
    /// 简易材质资产（Phase 1 Unlit；后续可升 Lit）。
    class MaterialAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::Material; }

        glm::vec4 AlbedoColor{1.0f, 1.0f, 1.0f, 1.0f};
        AssetHandle AlbedoTextureHandle = 0;
    };
}
