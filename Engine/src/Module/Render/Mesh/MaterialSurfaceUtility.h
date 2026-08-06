#pragma once

#include "Module/Render/Mesh/MaterialAsset.h"
#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include <glm/glm.hpp>

namespace Himii
{
    class AssetManager;
    class Texture2D;

    struct ResolvedMaterialSurface
    {
        glm::vec4 AlbedoColor{0.5f, 0.5f, 0.5f, 1.0f};
        float Specular = 0.5f;
        float Shininess = 32.0f;
        MaterialShadingMode ShadingMode = MaterialShadingMode::Lit;
        Ref<Texture2D> AlbedoTexture;
    };

    /// UE 风格空槽默认：引擎内置 Lit 灰表面（非磁盘资产）。
    ResolvedMaterialSurface GetEngineDefaultLitSurface();

    /// Handle 为 0 或无效时回退到 GetEngineDefaultLitSurface()。
    ResolvedMaterialSurface ResolveMaterialSurface(AssetManager *assetManager, AssetHandle materialHandle);
}
