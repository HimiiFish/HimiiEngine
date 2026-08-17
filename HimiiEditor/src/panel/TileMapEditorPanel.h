#pragma once

#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"

#include <glm/glm.hpp>

namespace Himii
{

    class TileMapEditorPanel {
    public:
        TileMapEditorPanel() = default;

        void OnImGuiRender(bool &isOpen);
        void Open(AssetHandle tileMapHandle);

        enum class Tool { Brush, Eraser, BoxFill, BoxErase };

        AssetHandle GetTileMapHandle() const { return m_TileMapHandle; }
        Ref<TileMapData> GetMapData() const { return m_MapData; }
        Ref<TileSet> GetTileSet() const { return m_TileSet; }
        Tool GetCurrentTool() const { return m_CurrentTool; }
        uint16_t GetSelectedTileID() const { return m_SelectedTileID; }
        bool HasBrushTileSelected() const { return m_BrushTileSelected; }
        bool CanPaintInScene() const;
        bool HasActiveTileMap() const { return m_MapData != nullptr; }

        bool IsMoveEntityModeEnabled() const { return m_MoveEntityModeEnabled; }
        void SetMoveEntityModeEnabled(bool enabled);
        void ToggleMoveEntityMode();

        void ApplyToolAtTile(int tileX, int tileY);
        void ApplyBoxTool(int minTileX, int minTileY, int maxTileX, int maxTileY);
        void SetCurrentTool(Tool tool) { m_CurrentTool = tool; }
        bool IsBoxTool(Tool tool) const
        {
            return tool == Tool::BoxFill || tool == Tool::BoxErase;
        }
        bool IsBoxToolActive() const { return IsBoxTool(m_CurrentTool); }
        glm::ivec2 GetHoveredTileCoordinates() const { return m_HoveredTileCoordinates; }
        void SetHoveredTileCoordinates(glm::ivec2 coordinates) { m_HoveredTileCoordinates = coordinates; }

        bool SaveActiveTileMapAssets();

        /// 清空笔刷并退出场景绘制会话（关闭 Setup、切换选中、移动实体模式时调用）
        void ClearScenePaintSession();

    private:
        static constexpr float LeftPanelWidth = 380.0f;

        void LoadAssets();
        void SaveTileMap();
        void SaveTileSet();
        void SliceAtlasGrid();

        void DrawLeftPanel();
        void DrawAtlasPreviewPanel();
        void DrawSelectedTileThumbnail();

        void UI_Overview();
        void UI_ScenePaint();
        void UI_Toolbar();
        void UI_Collision();
        void UI_AtlasSetup();
        void UI_BrushTile();
        void UI_RuleTiles();
        void UI_Properties();

        uint16_t FindTileIDAtAtlasCell(int column, int row) const;
        const TileDef* GetSelectedTileDefinition() const;
        RuleTileDefinition* GetSelectedRuleTileDefinition();
        const RuleTileDefinition* GetSelectedRuleTileDefinition() const;
        void SelectRuleTileAsBrush(uint16_t identifier);
        void DrawTileDefinitionThumbnail(uint16_t tileIdentifier, float thumbnailSize) const;
        void DrawRuleTileOutputIdentifierList(const char* label,
                                              std::vector<uint16_t>& outputTileIdentifiers,
                                              bool isAssignmentTarget);

        AssetHandle m_TileMapHandle = 0;
        Ref<TileMapData> m_MapData;
        Ref<TileSet> m_TileSet;

        Tool m_CurrentTool = Tool::Brush;
        uint16_t m_SelectedTileID = 0;
        bool m_BrushTileSelected = false;
        bool m_AssignAtlasClickToRuleTileOutput = false;
        int32_t m_SelectedRuleTileRuleIndex = -1;

        glm::ivec2 m_HoveredTileCoordinates{TileMapCoordinateUtility::InvalidTileCoordinate,
                                            TileMapCoordinateUtility::InvalidTileCoordinate};

        AssetHandle m_AtlasTextureHandle = 0;
        uint32_t m_AtlasTileSize = 16;
        bool m_MoveEntityModeEnabled = false;
    };

} // namespace Himii
