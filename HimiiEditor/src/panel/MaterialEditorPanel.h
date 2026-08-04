#pragma once

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/RenderCore/Texture.h"

namespace Himii
{
    class MaterialEditorPanel
    {
    public:
        void OnImGuiRender(bool &isOpen);

        void SetMaterialHandle(AssetHandle materialHandle);
        AssetHandle GetMaterialHandle() const { return m_MaterialHandle; }

        bool SaveActiveMaterialAsset();

    private:
        void ReloadMaterial();
        void DrawMaterialProperties();

        AssetHandle m_MaterialHandle = 0;
        Ref<MaterialAsset> m_MaterialAsset;
        Ref<Texture2D> m_AlbedoPreviewTexture;
        bool m_IsDirty = false;
    };
}
