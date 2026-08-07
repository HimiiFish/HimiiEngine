#pragma once

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Module/Render/Mesh/MaterialAsset.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Module/Render/RenderCore/Texture.h"

#include <unordered_set>

namespace Himii
{
    class MaterialEditorPanel
    {
    public:
        void OnImGuiRender(bool &isOpen);

        void SetMaterialHandle(AssetHandle materialHandle);
        AssetHandle GetMaterialHandle() const { return m_MaterialHandle; }

        bool HasDirtyMaterials() const { return !m_DirtyMaterialHandles.empty(); }
        size_t GetDirtyMaterialCount() const { return m_DirtyMaterialHandles.size(); }
        bool IsActiveMaterialDirty() const;

        int SaveAllDirtyMaterialAssets();
        void DiscardAllDirtyMaterialChanges();
        void ResetForProjectChange();

    private:
        void ReloadMaterial();
        void DrawMaterialProperties();
        void MarkActiveMaterialDirty();
        bool SaveMaterialAsset(AssetHandle materialHandle);

        AssetHandle m_MaterialHandle = 0;
        Ref<MaterialAsset> m_MaterialAsset;
        Ref<ShaderAsset> m_ShaderAsset;
        std::unordered_set<AssetHandle> m_DirtyMaterialHandles;
    };
}
