#pragma once

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Resource/Sprite.h"
#include "Module/Render/Texture.h"

#include <glm/glm.hpp>

namespace Himii
{

    class TextureInspectorPanel
    {
    public:
        void OnImGuiRender(bool& isOpen);

        void SetTextureHandle(AssetHandle textureHandle);
        AssetHandle GetTextureHandle() const { return m_TextureHandle; }

        /// Ctrl+S 时写入当前纹理的 .meta（若面板有打开的资源）
        bool SaveActiveTextureMeta();

    private:
        void DrawLeftInspectorPanel();
        void DrawRightPreviewPanel();
        void DrawImportSettings();
        void DrawSliceSettings();
        void DrawSelectedSpriteThumbnail(const SpriteDefinition& sprite);
        void ReloadImportData();
        void SyncUIToImportData();
        void SyncPendingEditsToMemory();
        void AddNewSprite();
        void DeleteSelectedSprite();

        bool UsesSliceWorkflow() const;

        int FindSpriteIndexAtPixel(const glm::ivec2& pixelCoordinate) const;

        static constexpr float LeftPanelWidth = 380.0f;

        AssetHandle m_TextureHandle = 0;
        Ref<Texture2D> m_PreviewTexture;

        int m_SelectedSpriteIndex = -1;
        bool m_IsDrawingSpriteRect = false;
        glm::ivec2 m_DrawStartPixel{0, 0};
        glm::ivec2 m_DrawEndPixel{0, 0};

        glm::ivec2 m_GridCellSize{32, 32};
        glm::ivec2 m_GridOffset{0, 0};
        glm::ivec2 m_GridPadding{0, 0};
        int m_SpriteModeSelection = 1;
        uint32_t m_PixelsPerUnit = 100;
        char m_SpriteNameEditBuffer[128]{};
    };

} // namespace Himii
