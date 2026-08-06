#include "Hepch.h"
#include "Module/Render/Mesh/MaterialSurfaceUtility.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Resource/AssetManager.h"

namespace Himii
{
    ResolvedMaterialSurface GetEngineDefaultLitSurface()
    {
        ResolvedMaterialSurface surface;
        surface.AlbedoColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        surface.Specular = 0.5f;
        surface.Shininess = 32.0f;
        surface.ShadingMode = MaterialShadingMode::Lit;
        return surface;
    }

    ResolvedMaterialSurface ResolveMaterialSurface(AssetManager *assetManager, AssetHandle materialHandle)
    {
        if (materialHandle == 0 || !assetManager)
            return GetEngineDefaultLitSurface();

        Ref<Asset> materialBase = assetManager->GetAsset(materialHandle);
        if (!materialBase || materialBase->GetType() != AssetType::Material)
            return GetEngineDefaultLitSurface();

        Ref<MaterialAsset> materialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);
        ResolvedMaterialSurface surface;
        surface.AlbedoColor = materialAsset->AlbedoColor;
        surface.Specular = materialAsset->Specular;
        surface.Shininess = materialAsset->Shininess;
        surface.ShadingMode = materialAsset->ShadingMode;

        if (materialAsset->AlbedoTextureHandle != 0)
        {
            Ref<Asset> textureBase = assetManager->GetAsset(materialAsset->AlbedoTextureHandle);
            if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                surface.AlbedoTexture = std::static_pointer_cast<Texture2D>(textureBase);
        }

        return surface;
    }
}
