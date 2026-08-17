#include "TileMapEditorPanel.h"
#include "InspectorControls.h"
#include "TilemapEditorUtility.h"

#include "imgui.h"
#include "Resource/AssetManager.h"
#include "Resource/AssetSerializer.h"
#include "Resource/SpriteSheetUtility.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "EngineCore/Core/Log.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace Himii
{

    void TileMapEditorPanel::Open(AssetHandle tileMapHandle)
    {
        m_TileMapHandle = tileMapHandle;
        m_MapData = nullptr;
        m_TileSet = nullptr;
        m_AtlasTextureHandle = 0;
        m_SelectedTileID = 0;
        m_BrushTileSelected = false;
        m_AssignAtlasClickToRuleTileOutput = false;
        m_SelectedRuleTileRuleIndex = -1;
        LoadAssets();
    }

    void TileMapEditorPanel::ClearScenePaintSession()
    {
        m_SelectedTileID = 0;
        m_BrushTileSelected = false;
        m_AssignAtlasClickToRuleTileOutput = false;
        m_SelectedRuleTileRuleIndex = -1;
        m_CurrentTool = Tool::Brush;
        m_MoveEntityModeEnabled = false;
        m_HoveredTileCoordinates = {TileMapCoordinateUtility::InvalidTileCoordinate,
                                    TileMapCoordinateUtility::InvalidTileCoordinate};
    }

    void TileMapEditorPanel::SetMoveEntityModeEnabled(bool enabled)
    {
        if (enabled)
        {
            m_SelectedTileID = 0;
            m_BrushTileSelected = false;
            m_AssignAtlasClickToRuleTileOutput = false;
            m_SelectedRuleTileRuleIndex = -1;
            m_CurrentTool = Tool::Brush;
        }
        m_MoveEntityModeEnabled = enabled;
    }

    void TileMapEditorPanel::ToggleMoveEntityMode()
    {
        SetMoveEntityModeEnabled(!m_MoveEntityModeEnabled);
    }

    bool TileMapEditorPanel::CanPaintInScene() const
    {
        if (!m_MapData || m_MoveEntityModeEnabled)
            return false;

        if (m_CurrentTool == Tool::Eraser || m_CurrentTool == Tool::BoxErase)
            return true;

        if (!m_BrushTileSelected || m_SelectedTileID == 0 || !m_TileSet)
            return false;

        if (m_TileSet->GetRuleTileDefinition(m_SelectedTileID))
            return m_TileSet->HasPaintableRuleTile(m_SelectedTileID);

        return m_TileSet->GetTileDef(m_SelectedTileID) != nullptr;
    }

    void TileMapEditorPanel::LoadAssets()
    {
        if (!Project::GetActive() || m_TileMapHandle == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        auto mapAsset = assetManager->GetAsset(m_TileMapHandle);
        if (mapAsset)
            m_MapData = std::static_pointer_cast<TileMapData>(mapAsset);

        if (m_MapData && m_MapData->GetTileSetHandle() != 0)
        {
            auto tileSetAsset = assetManager->GetAsset(m_MapData->GetTileSetHandle());
            if (tileSetAsset)
            {
                m_TileSet = std::static_pointer_cast<TileSet>(tileSetAsset);
                if (!m_TileSet->GetAtlasSources().empty())
                {
                    m_AtlasTextureHandle = m_TileSet->GetAtlasSources()[0].TextureHandle;
                    m_AtlasTileSize = m_TileSet->GetAtlasSources()[0].TileSize;
                }
            }
        }
    }

    const TileDef* TileMapEditorPanel::GetSelectedTileDefinition() const
    {
        if (!m_TileSet)
            return nullptr;
        return m_TileSet->GetTileDef(m_SelectedTileID);
    }

    RuleTileDefinition* TileMapEditorPanel::GetSelectedRuleTileDefinition()
    {
        if (!m_TileSet)
            return nullptr;
        return m_TileSet->GetRuleTileDefinition(m_SelectedTileID);
    }

    const RuleTileDefinition* TileMapEditorPanel::GetSelectedRuleTileDefinition() const
    {
        if (!m_TileSet)
            return nullptr;
        return m_TileSet->GetRuleTileDefinition(m_SelectedTileID);
    }

    void TileMapEditorPanel::SelectRuleTileAsBrush(uint16_t identifier)
    {
        m_SelectedTileID = identifier;
        m_BrushTileSelected = identifier != 0;
        m_AssignAtlasClickToRuleTileOutput = identifier != 0;
        m_SelectedRuleTileRuleIndex = -1;
        if (m_CurrentTool == Tool::Eraser || m_CurrentTool == Tool::BoxErase)
            m_CurrentTool = Tool::Brush;
    }

    uint16_t TileMapEditorPanel::FindTileIDAtAtlasCell(int column, int row) const
    {
        if (!m_TileSet)
            return 0;

        for (const auto& [tileIdentifier, tileDefinition] : m_TileSet->GetTileDefs())
        {
            if (tileDefinition.SourceType == TileSourceType::Atlas
                && tileDefinition.AtlasSourceIndex == 0
                && tileDefinition.AtlasCoords.x == column
                && tileDefinition.AtlasCoords.y == row)
            {
                return tileIdentifier;
            }
        }

        return 0;
    }

    void TileMapEditorPanel::OnImGuiRender(bool &isOpen)
    {
        if (!isOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(960.0f, 600.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("TileMap Setup", &isOpen))
        {
            ImGui::End();
            return;
        }

        if (m_TileMapHandle == 0 || !m_MapData)
        {
            ImGui::TextWrapped(
                "未加载 TileMap。请选中带 TilemapComponent 的实体，或在 Inspector 中打开 TileMap Setup。");
            ImGui::End();
            return;
        }

        if (ImGui::BeginTable("##TileMapSetupSplit", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
                                  | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, LeftPanelWidth);
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            {
                const ImVec2 leftPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##TileMapSetupLeft", leftPanelSize, false,
                                      ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    DrawLeftPanel();
                }
                ImGui::EndChild();
            }

            ImGui::TableNextColumn();
            {
                const ImVec2 rightPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##TileMapSetupRight", rightPanelSize, true,
                                      ImGuiWindowFlags_NoScrollbar))
                {
                    DrawAtlasPreviewPanel();
                }
                ImGui::EndChild();
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void TileMapEditorPanel::DrawLeftPanel()
    {
        UI_Overview();
        UI_ScenePaint();
        UI_Toolbar();
        UI_Collision();
        UI_AtlasSetup();
        UI_BrushTile();
        UI_RuleTiles();
        UI_Properties();
    }

    void TileMapEditorPanel::UI_Overview()
    {
        DrawInspectorSectionHeader(
                "TileMap",
                "在右侧图集点选图块，再在 Scene 中绘制。Ctrl+S 保存 TileMap 与 TileSet。");

        std::string tileMapFileName = "Unknown";
        if (auto assetManager = ResourceSystem::GetAssetManager())
        {
            const auto& registry = assetManager->GetAssetRegistry();
            const auto iterator = registry.find(m_TileMapHandle);
            if (iterator != registry.end())
                tileMapFileName = iterator->second.FilePath.filename().string();
        }

        DrawReadOnlyTextControl("Asset", tileMapFileName.c_str(),
                                "当前编辑的 TileMap 资产文件。");

        char extentBuffer[64] = {};
        std::snprintf(extentBuffer, sizeof(extentBuffer), "%u x %u tiles",
                      m_MapData->GetWidth(), m_MapData->GetHeight());
        DrawReadOnlyTextControl("Extent", extentBuffer,
                                "已绘制区域在瓦片坐标下的宽高（格数）。");

        char cellSizeBuffer[32] = {};
        std::snprintf(cellSizeBuffer, sizeof(cellSizeBuffer), "%.2f", m_MapData->GetCellSize());
        DrawReadOnlyTextControl("Cell Size", cellSizeBuffer,
                                "每个瓦片在世界空间中的边长（与 Transform 缩放相乘）。");
    }

    void TileMapEditorPanel::UI_ScenePaint()
    {
        DrawInspectorSectionHeader("Scene Paint",
                                 "在 Scene 中绘制瓦片；开启 Move Entity 后可用变换 Gizmo 移动实体。");

        if (DrawEditorToggleButton(
                m_MoveEntityModeEnabled ? "Move Entity: ON [H]" : "Move Entity: OFF [H]",
                m_MoveEntityModeEnabled,
                ImVec2(-1.0f, 0.0f),
                "切换后可在 Scene 中移动带 Tilemap 的实体。快捷键 H。"))
        {
            ToggleMoveEntityMode();
        }
    }

    void TileMapEditorPanel::UI_Toolbar()
    {
        DrawInspectorSectionHeader(
                "Paint Tools",
                "笔刷/橡皮擦：点击或拖动绘制。框选工具：在 Scene 中拖拽矩形后松开应用。");

        const bool isBrush = (m_CurrentTool == Tool::Brush);
        const bool isEraser = (m_CurrentTool == Tool::Eraser);
        const bool isBoxFill = (m_CurrentTool == Tool::BoxFill);
        const bool isBoxErase = (m_CurrentTool == Tool::BoxErase);

        if (ImGui::BeginTable("##PaintTools", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextColumn();
            if (DrawEditorToggleButton("Brush [B]", isBrush, ImVec2(-FLT_MIN, 0.0f),
                                       "绘制当前选中的图块。快捷键 B。"))
                m_CurrentTool = Tool::Brush;

            ImGui::TableNextColumn();
            if (DrawEditorToggleButton("Eraser [E]", isEraser, ImVec2(-FLT_MIN, 0.0f),
                                       "擦除瓦片（设为 Empty）。快捷键 E。"))
                m_CurrentTool = Tool::Eraser;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            if (DrawEditorToggleButton("Box Fill [G]", isBoxFill, ImVec2(-FLT_MIN, 0.0f),
                                       "拖拽框选矩形，用当前笔刷填充区域内全部瓦片。快捷键 G。"))
                m_CurrentTool = Tool::BoxFill;

            ImGui::TableNextColumn();
            if (DrawEditorToggleButton("Box Erase [Shift+E]", isBoxErase, ImVec2(-FLT_MIN, 0.0f),
                                       "拖拽框选矩形，擦除区域内全部瓦片。快捷键 Shift+E。"))
                m_CurrentTool = Tool::BoxErase;

            ImGui::EndTable();
        }
    }

    void TileMapEditorPanel::UI_Collision()
    {
        DrawInspectorSectionHeader("Collision");

        if (!m_TileSet)
        {
            ImGui::TextDisabled("Assign a TileSet to configure collision.");
            return;
        }

        uint32_t collidableDefinitionCount = 0;
        uint32_t totalDefinitionCount = static_cast<uint32_t>(m_TileSet->GetTileDefs().size()
                + m_TileSet->GetRuleTileDefinitions().size());
        for (const auto& [tileIdentifier, tileDefinition] : m_TileSet->GetTileDefs())
        {
            if (tileDefinition.Collidable)
                collidableDefinitionCount++;
        }
        for (const auto& [ruleTileIdentifier, ruleTileDefinition] : m_TileSet->GetRuleTileDefinitions())
        {
            if (ruleTileDefinition.Collidable)
                collidableDefinitionCount++;
        }

        char collidableStatsBuffer[48] = {};
        std::snprintf(collidableStatsBuffer, sizeof(collidableStatsBuffer), "%u / %u",
                      collidableDefinitionCount, totalDefinitionCount);
        DrawReadOnlyTextControl(
                "Collidable Types",
                collidableStatsBuffer,
                "Collidable 按图块类型生效（非单格绘制）。Play 前请保存 TileSet。");

        DrawHorizontalButtonPair("CollisionBatch", [&]()
        {
            if (DrawTableFillButton("Set All", "将全部图块类型标记为可碰撞。"))
            {
                for (auto& [tileIdentifier, tileDefinition] : m_TileSet->GetTileDefs())
                    tileDefinition.Collidable = true;
                for (auto& [ruleTileIdentifier, ruleTileDefinition] : m_TileSet->GetRuleTileDefinitions())
                    ruleTileDefinition.Collidable = true;
                SaveTileSet();
            }
        }, [&]()
        {
            if (DrawTableFillButton("Clear All", "清除全部图块类型的碰撞标记。"))
            {
                for (auto& [tileIdentifier, tileDefinition] : m_TileSet->GetTileDefs())
                    tileDefinition.Collidable = false;
                for (auto& [ruleTileIdentifier, ruleTileDefinition] : m_TileSet->GetRuleTileDefinitions())
                    ruleTileDefinition.Collidable = false;
                SaveTileSet();
            }
        });

        if (m_SelectedTileID != 0)
        {
            auto& tileDefinitions = m_TileSet->GetTileDefs();
            const auto iterator = tileDefinitions.find(m_SelectedTileID);
            if (iterator != tileDefinitions.end())
            {
                DrawPropertyRow("Selected Tile Collidable", [&]()
                {
                    ImGui::Checkbox("##Value", &iterator->second.Collidable);
                }, "仅影响当前在图集中选中的图块类型。");
                if (ImGui::IsItemDeactivatedAfterEdit())
                    SaveTileSet();
            }
            else if (RuleTileDefinition* ruleTileDefinition = GetSelectedRuleTileDefinition())
            {
                DrawCheckboxControl("Selected Tile Collidable", ruleTileDefinition->Collidable, false);
            }
        }
    }

    void TileMapEditorPanel::UI_AtlasSetup()
    {
        DrawInspectorSectionHeader(
                "Atlas Grid",
                "可将纹理从内容浏览器拖入本区或右侧预览区。Slice Grid 按像素网格生成图块定义。");

        if (!m_TileSet)
        {
            ImGui::TextDisabled("No TileSet assigned to this TileMap.");
            return;
        }

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        DrawPropertyRow("Tile Size (px)", [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            ImGui::DragScalar("##Value", ImGuiDataType_U32, &m_AtlasTileSize, 1.0f, nullptr, nullptr, "%u");
            ImGui::PopItemWidth();
        }, "图集中每个瓦片在纹理上的像素边长。");
        if (!m_TileSet->GetAtlasSources().empty())
            m_TileSet->GetAtlasSources()[0].TileSize = m_AtlasTileSize;

        DrawActionButtonRow("Atlas", [&]()
        {
            if (DrawTableFillButton("Slice Grid", "按当前 Tile Size 切分图集并生成/更新图块 ID。"))
                SliceAtlasGrid();
        });

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t *path = (const wchar_t *)payload->Data;
                std::filesystem::path assetPath = path;
                std::string extension = assetPath.extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(),
                               [](unsigned char character)
                               {
                                   return (char)std::tolower(character);
                               });
                if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
                {
                    AssetHandle textureHandle = assetManager->ImportAsset(assetPath);
                    if (textureHandle != 0)
                    {
                        m_AtlasTextureHandle = textureHandle;
                        TileAtlasSource atlasSource;
                        atlasSource.TextureHandle = textureHandle;
                        atlasSource.TileSize = m_AtlasTileSize;
                        m_TileSet->GetAtlasSources().clear();
                        m_TileSet->AddAtlasSource(atlasSource);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void TileMapEditorPanel::UI_BrushTile()
    {
        DrawInspectorSectionHeader("Brush",
                                 "在右侧图集预览中点击单元格以选择普通图块笔刷；Rule Tile 请在下方列表中选择。");

        if (!m_TileSet)
            return;

        if (const RuleTileDefinition* ruleTileDefinition = GetSelectedRuleTileDefinition())
        {
            const char* displayName = ruleTileDefinition->DisplayName.empty()
                    ? "Rule Tile"
                    : ruleTileDefinition->DisplayName.c_str();
            DrawReadOnlyTextControl("Selected Rule Tile", displayName,
                                    "格子将写入该 Rule Tile 的逻辑 ID，绘制时按邻居解析外观。");
            if (!m_TileSet->HasPaintableRuleTile(m_SelectedTileID))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                                   "Add at least one default output tile before painting.");
            }
            else
            {
                for (uint16_t outputTileIdentifier : ruleTileDefinition->DefaultOutputTileIdentifiers)
                {
                    if (m_TileSet->GetTileDef(outputTileIdentifier))
                    {
                        DrawPropertyRow("Default Preview", [&]()
                        {
                            DrawTileDefinitionThumbnail(outputTileIdentifier, 72.0f);
                        }, "默认输出图块预览（规则全未匹配时使用）。");
                        break;
                    }
                }
            }
            return;
        }

        const TileDef* selectedTileDefinition = GetSelectedTileDefinition();
        if (!selectedTileDefinition)
            return;

        DrawSelectedTileThumbnail();
    }

    void TileMapEditorPanel::DrawSelectedTileThumbnail()
    {
        const TileDef* selectedTileDefinition = GetSelectedTileDefinition();
        if (!selectedTileDefinition || !m_TileSet)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || m_AtlasTextureHandle == 0)
            return;

        Ref<Texture2D> atlasTexture =
                std::static_pointer_cast<Texture2D>(assetManager->GetAsset(m_AtlasTextureHandle));
        if (!atlasTexture || selectedTileDefinition->SourceType != TileSourceType::Atlas)
            return;

        const uint32_t tilePixelSize = m_AtlasTileSize > 0 ? m_AtlasTileSize : 16;
        const glm::ivec4 pixelRect = SpriteSheetUtility::AtlasGridCoordsToPixelRect(
                selectedTileDefinition->AtlasCoords, tilePixelSize);

        const float thumbnailSize = 72.0f;
        DrawPropertyRow("Selected Tile", [&]()
        {
            DrawEditorTextureImageSubRect(
                atlasTexture->GetRendererID(), ImVec2(thumbnailSize, thumbnailSize),
                pixelRect, atlasTexture->GetWidth(), atlasTexture->GetHeight());
        }, "当前笔刷图块在图集中的外观预览。");
    }

    void TileMapEditorPanel::DrawAtlasPreviewPanel()
    {
        DrawInspectorSectionHeader(
                "Atlas Preview",
                m_AssignAtlasClickToRuleTileOutput
                        ? "点击单元格，将普通图块追加到当前 Rule Tile 的输出列表。"
                        : "点击单元格选择笔刷；可拖入 PNG/JPG 纹理。未 Slice 时显示提示叠加。");

        if (!m_TileSet)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        Ref<Texture2D> atlasTexture;
        if (m_AtlasTextureHandle != 0)
            atlasTexture = std::static_pointer_cast<Texture2D>(assetManager->GetAsset(m_AtlasTextureHandle));

        if (!atlasTexture)
        {
            const ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("##AtlasPreviewDropZone", dropZoneSize);
            DrawInspectorTooltipIfHovered("将纹理从内容浏览器拖入此区域。");

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const wchar_t *path = (const wchar_t *)payload->Data;
                    std::filesystem::path assetPath = path;
                    std::string extension = assetPath.extension().string();
                    std::transform(extension.begin(), extension.end(), extension.begin(),
                                   [](unsigned char character)
                                   {
                                       return (char)std::tolower(character);
                                   });
                    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
                    {
                        AssetHandle textureHandle = assetManager->ImportAsset(assetPath);
                        if (textureHandle != 0)
                        {
                            m_AtlasTextureHandle = textureHandle;
                            TileAtlasSource atlasSource;
                            atlasSource.TextureHandle = textureHandle;
                            atlasSource.TileSize = m_AtlasTileSize;
                            m_TileSet->GetAtlasSources().clear();
                            m_TileSet->AddAtlasSource(atlasSource);
                            atlasTexture = std::static_pointer_cast<Texture2D>(
                                    assetManager->GetAsset(m_AtlasTextureHandle));
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            return;
        }

        const float previewWidth = ImGui::GetContentRegionAvail().x;
        const float previewHeight = ImGui::GetContentRegionAvail().y;
        const float textureAspect = static_cast<float>(atlasTexture->GetWidth())
            / static_cast<float>(atlasTexture->GetHeight());
        const float panelAspect = previewWidth / previewHeight;

        float displayWidth = previewWidth;
        float displayHeight = previewHeight;
        if (textureAspect > panelAspect)
            displayHeight = displayWidth / textureAspect;
        else
            displayWidth = displayHeight * textureAspect;

        const float offsetX = (previewWidth - displayWidth) * 0.5f;
        const float offsetY = (previewHeight - displayHeight) * 0.5f;
        ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + offsetX, ImGui::GetCursorPosY() + offsetY));

        const ImVec2 imagePosition = ImGui::GetCursorScreenPos();
        DrawEditorTextureImageFull(atlasTexture->GetRendererID(), ImVec2(displayWidth, displayHeight));

        const uint32_t tilePixelSize = m_AtlasTileSize > 0 ? m_AtlasTileSize : 16;
        const uint32_t columnCount =
                std::max(1u, atlasTexture->GetWidth() / tilePixelSize);
        const uint32_t rowCount =
                std::max(1u, atlasTexture->GetHeight() / tilePixelSize);

        const float scaleX = displayWidth / static_cast<float>(atlasTexture->GetWidth());
        const float scaleY = displayHeight / static_cast<float>(atlasTexture->GetHeight());
        const float cellScreenWidth = static_cast<float>(tilePixelSize) * scaleX;
        const float cellScreenHeight = static_cast<float>(tilePixelSize) * scaleY;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (uint32_t row = 0; row < rowCount; ++row)
        {
            for (uint32_t column = 0; column < columnCount; ++column)
            {
                const ImVec2 rectMin(
                        imagePosition.x + static_cast<float>(column) * cellScreenWidth,
                        imagePosition.y + static_cast<float>(row) * cellScreenHeight);
                const ImVec2 rectMax(rectMin.x + cellScreenWidth, rectMin.y + cellScreenHeight);

                const uint16_t cellTileIdentifier = FindTileIDAtAtlasCell(static_cast<int>(column),
                                                                          static_cast<int>(row));
                const bool isSelectedCell = cellTileIdentifier != 0
                    && cellTileIdentifier == m_SelectedTileID;

                if (cellTileIdentifier != 0)
                    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(0, 200, 255, 20));

                if (isSelectedCell)
                {
                    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(255, 220, 0, 45));
                    drawList->AddRect(rectMin, rectMax, IM_COL32(255, 220, 0, 255), 0.0f, 0, 2.0f);
                }
                else
                    drawList->AddRect(rectMin, rectMax, IM_COL32(100, 100, 100, 120), 0.0f, 0, 1.0f);
            }
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const ImVec2 mousePosition = ImGui::GetMousePos();
            const float localX = mousePosition.x - imagePosition.x;
            const float localY = mousePosition.y - imagePosition.y;

            if (localX >= 0.0f && localY >= 0.0f && localX < displayWidth && localY < displayHeight)
            {
                const int column = static_cast<int>(localX / cellScreenWidth);
                const int row = static_cast<int>(localY / cellScreenHeight);

                if (column >= 0 && row >= 0
                    && column < static_cast<int>(columnCount)
                    && row < static_cast<int>(rowCount))
                {
                    const uint16_t pickedTileIdentifier = FindTileIDAtAtlasCell(column, row);
                    if (pickedTileIdentifier != 0)
                    {
                        RuleTileDefinition* ruleTileDefinition = GetSelectedRuleTileDefinition();
                        if (m_AssignAtlasClickToRuleTileOutput && ruleTileDefinition)
                        {
                            std::vector<uint16_t>* outputTileIdentifiers =
                                    &ruleTileDefinition->DefaultOutputTileIdentifiers;
                            if (m_SelectedRuleTileRuleIndex >= 0
                                && m_SelectedRuleTileRuleIndex
                                        < static_cast<int32_t>(ruleTileDefinition->Rules.size()))
                            {
                                outputTileIdentifiers = &ruleTileDefinition->Rules
                                        [static_cast<size_t>(m_SelectedRuleTileRuleIndex)]
                                                .OutputTileIdentifiers;
                            }

                            if (std::find(outputTileIdentifiers->begin(), outputTileIdentifiers->end(),
                                          pickedTileIdentifier)
                                == outputTileIdentifiers->end())
                            {
                                outputTileIdentifiers->push_back(pickedTileIdentifier);
                            }
                        }
                        else
                        {
                            m_SelectedTileID = pickedTileIdentifier;
                            m_BrushTileSelected = true;
                            m_AssignAtlasClickToRuleTileOutput = false;
                            m_SelectedRuleTileRuleIndex = -1;
                            if (m_CurrentTool == Tool::Eraser || m_CurrentTool == Tool::BoxErase)
                                m_CurrentTool = Tool::Brush;
                        }
                    }
                }
            }
        }

        if (m_TileSet->GetTileDefs().empty())
        {
            ImGui::SetCursorScreenPos(imagePosition);
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "Click Slice Grid to generate tiles.");
        }
    }

    void TileMapEditorPanel::SliceAtlasGrid()
    {
        if (!m_TileSet || m_AtlasTextureHandle == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        Ref<Texture2D> texture =
                std::static_pointer_cast<Texture2D>(assetManager->GetAsset(m_AtlasTextureHandle));
        if (!texture)
            return;

        if (m_TileSet->GetAtlasSources().empty())
        {
            TileAtlasSource atlasSource;
            atlasSource.TextureHandle = m_AtlasTextureHandle;
            atlasSource.TileSize = m_AtlasTileSize;
            m_TileSet->AddAtlasSource(atlasSource);
        }
        else
        {
            m_TileSet->GetAtlasSources()[0].TextureHandle = m_AtlasTextureHandle;
            m_TileSet->GetAtlasSources()[0].TileSize = m_AtlasTileSize;
        }

        const std::unordered_map<uint16_t, TileDef> previousDefinitions = m_TileSet->GetTileDefs();

        const uint32_t tileSize = m_AtlasTileSize > 0 ? m_AtlasTileSize : 16;
        const uint32_t columnCount = std::max(1u, texture->GetWidth() / tileSize);
        const uint32_t rowCount = std::max(1u, texture->GetHeight() / tileSize);
        m_TileSet->GenerateGridTileDefs(0, columnCount, rowCount, &previousDefinitions);

        m_SelectedTileID = 0;
        m_BrushTileSelected = false;
        m_AssignAtlasClickToRuleTileOutput = false;
        m_SelectedRuleTileRuleIndex = -1;

        HIMII_CORE_INFO("TileSet sliced: {0}x{1} tiles", columnCount, rowCount);
    }

    void TileMapEditorPanel::DrawTileDefinitionThumbnail(uint16_t tileIdentifier, float thumbnailSize) const
    {
        if (!m_TileSet)
            return;

        const TileDef* tileDefinition = m_TileSet->GetTileDef(tileIdentifier);
        if (!tileDefinition || tileDefinition->SourceType != TileSourceType::Atlas)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || m_AtlasTextureHandle == 0)
            return;

        Ref<Texture2D> atlasTexture =
                std::static_pointer_cast<Texture2D>(assetManager->GetAsset(m_AtlasTextureHandle));
        if (!atlasTexture)
            return;

        const uint32_t tilePixelSize = m_AtlasTileSize > 0 ? m_AtlasTileSize : 16;
        const glm::ivec4 pixelRect = SpriteSheetUtility::AtlasGridCoordsToPixelRect(
                tileDefinition->AtlasCoords, tilePixelSize);
        DrawEditorTextureImageSubRect(
                atlasTexture->GetRendererID(), ImVec2(thumbnailSize, thumbnailSize), pixelRect,
                atlasTexture->GetWidth(), atlasTexture->GetHeight());
    }

    void TileMapEditorPanel::DrawRuleTileOutputIdentifierList(
            const char* label,
            std::vector<uint16_t>& outputTileIdentifiers,
            bool isAssignmentTarget)
    {
        DrawPropertyRow(
                label,
                [&]()
                {
                    if (outputTileIdentifiers.empty())
                    {
                        ImGui::TextDisabled(isAssignmentTarget ? "Click atlas to add" : "None");
                        return;
                    }

                    const float thumbnailSize = 36.0f;
                    for (size_t outputIndex = 0; outputIndex < outputTileIdentifiers.size(); ++outputIndex)
                    {
                        ImGui::PushID(static_cast<int>(outputIndex));
                        DrawTileDefinitionThumbnail(outputTileIdentifiers[outputIndex], thumbnailSize);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Remove"))
                        {
                            outputTileIdentifiers.erase(outputTileIdentifiers.begin()
                                    + static_cast<std::ptrdiff_t>(outputIndex));
                            ImGui::PopID();
                            break;
                        }
                        ImGui::PopID();
                    }
                },
                isAssignmentTarget ? "当前正在接收图集点选。输出必须是普通图块。"
                                   : "从图集点选追加输出图块。");
    }

    void TileMapEditorPanel::UI_RuleTiles()
    {
        DrawInspectorSectionHeader(
                "Rule Tiles",
                "Rule Tile 把逻辑 ID 写进地图；外观按 3×3 邻居在绘制时解析。Slice Grid 不会删除这些规则。");

        if (!m_TileSet)
        {
            ImGui::TextDisabled("Assign a TileSet to configure Rule Tiles.");
            return;
        }

        DrawHorizontalButtonPair(
                "RuleTileListActions",
                [&]()
                {
                    if (DrawTableFillButton("New Rule Tile", "分配新的逻辑 ID 并加入 TileSet。"))
                    {
                        const uint16_t identifier = m_TileSet->GetNextTileIdentifier();
                        if (identifier != 0)
                        {
                            RuleTileDefinition definition;
                            definition.Identifier = identifier;
                            definition.DisplayName = "Rule Tile";
                            m_TileSet->AddRuleTileDefinition(definition);
                            SelectRuleTileAsBrush(identifier);
                        }
                    }
                },
                [&]()
                {
                    if (DrawTableFillButton("Delete Rule Tile",
                                            "从 TileSet 删除选中的 Rule Tile，不改写地图上已刷的格子。"))
                    {
                        if (GetSelectedRuleTileDefinition())
                        {
                            m_TileSet->RemoveRuleTileDefinition(m_SelectedTileID);
                            SelectRuleTileAsBrush(0);
                            m_BrushTileSelected = false;
                            m_AssignAtlasClickToRuleTileOutput = false;
                        }
                    }
                });

        std::vector<uint16_t> ruleTileIdentifiers;
        ruleTileIdentifiers.reserve(m_TileSet->GetRuleTileDefinitions().size());
        for (const auto& [identifier, definition] : m_TileSet->GetRuleTileDefinitions())
            ruleTileIdentifiers.push_back(identifier);
        std::sort(ruleTileIdentifiers.begin(), ruleTileIdentifiers.end());

        if (ruleTileIdentifiers.empty())
            ImGui::TextDisabled("No Rule Tiles yet.");

        for (uint16_t identifier : ruleTileIdentifiers)
        {
            const RuleTileDefinition* definition = m_TileSet->GetRuleTileDefinition(identifier);
            if (!definition)
                continue;

            ImGui::PushID(static_cast<int>(identifier));
            const bool isSelected = m_BrushTileSelected && m_SelectedTileID == identifier;
            char ruleTileLabel[160] = {};
            if (definition->DisplayName.empty())
                std::snprintf(ruleTileLabel, sizeof(ruleTileLabel), "Rule Tile %u", identifier);
            else
                std::snprintf(ruleTileLabel, sizeof(ruleTileLabel), "%s", definition->DisplayName.c_str());

            if (DrawEditorToggleButton(ruleTileLabel, isSelected, ImVec2(-FLT_MIN, 0.0f),
                                       "选中后作为 Scene 笔刷，并编辑其规则。"))
            {
                SelectRuleTileAsBrush(identifier);
            }
            ImGui::PopID();
        }

        RuleTileDefinition* selectedRuleTileDefinition = GetSelectedRuleTileDefinition();
        if (!selectedRuleTileDefinition)
            return;

        DrawStdStringControl("Display Name", selectedRuleTileDefinition->DisplayName);
        DrawCheckboxControl("Collidable", selectedRuleTileDefinition->Collidable, false);
        DrawCheckboxControl("Atlas Click Assigns Output", m_AssignAtlasClickToRuleTileOutput, false);

        if (ImGui::Button("Assign Defaults", ImVec2(-FLT_MIN, 0.0f)))
            m_SelectedRuleTileRuleIndex = -1;
        DrawInspectorTooltipIfHovered("之后点选图集，将图块加入默认输出列表。");

        DrawRuleTileOutputIdentifierList("Default Outputs",
                                         selectedRuleTileDefinition->DefaultOutputTileIdentifiers,
                                         m_SelectedRuleTileRuleIndex < 0);

        if (!m_TileSet->HasPaintableRuleTile(selectedRuleTileDefinition->Identifier))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                               "Cannot paint until a default output tile is assigned.");
        }

        DrawActionButtonRow("Rules", [&]()
        {
            if (DrawTableFillButton("Add Rule", "添加一条 3×3 匹配规则；列表中先匹配的先生效。"))
            {
                selectedRuleTileDefinition->Rules.emplace_back();
                m_SelectedRuleTileRuleIndex =
                        static_cast<int32_t>(selectedRuleTileDefinition->Rules.size()) - 1;
                m_AssignAtlasClickToRuleTileOutput = true;
            }
        });

        static const char* matchTransformLabels[] = {
                "Fixed", "Rotated", "Mirror Horizontal", "Mirror Vertical", "Mirror Both"};

        for (int32_t ruleIndex = 0;
             ruleIndex < static_cast<int32_t>(selectedRuleTileDefinition->Rules.size()); ++ruleIndex)
        {
            RuleTileRule& rule = selectedRuleTileDefinition->Rules[static_cast<size_t>(ruleIndex)];
            ImGui::PushID(ruleIndex);

            char ruleHeader[48] = {};
            std::snprintf(ruleHeader, sizeof(ruleHeader), "Rule %d", ruleIndex + 1);
            const bool isAssignmentTarget = m_SelectedRuleTileRuleIndex == ruleIndex;
            if (DrawEditorToggleButton(ruleHeader, isAssignmentTarget, ImVec2(-FLT_MIN, 0.0f),
                                       "选中后，图集点选会写入这条规则的输出。"))
            {
                m_SelectedRuleTileRuleIndex = ruleIndex;
                m_AssignAtlasClickToRuleTileOutput = true;
            }

            int matchTransformIndex = static_cast<int>(rule.MatchTransform);
            DrawEnumComboControl(
                    "Transform", matchTransformIndex, matchTransformLabels, 5,
                    [&](int newIndex)
                    {
                        rule.MatchTransform = static_cast<RuleTileMatchTransform>(newIndex);
                    });

            DrawRuleTileNeighborMatrixControl("Neighbors", rule.NeighborConditions);
            DrawRuleTileOutputIdentifierList("Outputs", rule.OutputTileIdentifiers, isAssignmentTarget);

            if (DrawTableFillButton("Delete Rule", "删除这条匹配规则。"))
            {
                selectedRuleTileDefinition->Rules.erase(selectedRuleTileDefinition->Rules.begin()
                        + static_cast<std::ptrdiff_t>(ruleIndex));
                if (m_SelectedRuleTileRuleIndex == ruleIndex)
                    m_SelectedRuleTileRuleIndex = -1;
                else if (m_SelectedRuleTileRuleIndex > ruleIndex)
                    m_SelectedRuleTileRuleIndex--;
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
    }

    void TileMapEditorPanel::UI_Properties()
    {
        DrawInspectorSectionHeader("Map Properties",
                                 "编辑地图数据与保存。Ctrl+S 会保存工程、TileMap 与 TileSet。");
        if (!m_MapData)
            return;

        if (m_MapData->HasBounds())
        {
            int32_t minTileX = 0;
            int32_t minTileY = 0;
            int32_t maxTileX = 0;
            int32_t maxTileY = 0;
            m_MapData->GetBounds(minTileX, minTileY, maxTileX, maxTileY);

            char boundsBuffer[64] = {};
            std::snprintf(boundsBuffer, sizeof(boundsBuffer), "(%d,%d) .. (%d,%d)",
                          minTileX, minTileY, maxTileX, maxTileY);
            DrawReadOnlyTextControl("Bounds", boundsBuffer,
                                    "已绘制瓦片在网格坐标下的最小/最大范围（含端点）。");
        }
        else
        {
            DrawReadOnlyTextControl("Bounds", "Empty",
                                    "尚无瓦片；在 Scene 中绘制后会自动扩展范围。");
        }

        float cellSize = m_MapData->GetCellSize();
        DrawPropertyRow("Cell Size", [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            ImGui::DragFloat("##Value", &cellSize, 0.05f, 0.1f, 10.0f);
            ImGui::PopItemWidth();
        }, "每个瓦片在世界空间中的边长（与实体 Transform 缩放相乘）。");
        m_MapData->SetCellSize(cellSize);

        DrawHorizontalButtonPair("SaveAssets", [&]()
        {
            if (DrawTableFillButton("Save TileMap", "将当前地图瓦片数据写入 .tilemap 文件。"))
                SaveTileMap();
        }, [&]()
        {
            if (DrawTableFillButton("Save TileSet", "将图集、图块定义、Rule Tile 与 Collidable 写入 .tileset 文件。"))
                SaveTileSet();
        });
    }

    void TileMapEditorPanel::ApplyToolAtTile(int tileX, int tileY)
    {
        if (!m_MapData)
            return;

        switch (m_CurrentTool)
        {
            case Tool::Brush:
                m_MapData->SetTile(tileX, tileY, m_SelectedTileID);
                break;
            case Tool::Eraser:
                m_MapData->SetTile(tileX, tileY, 0);
                break;
            case Tool::BoxFill:
            case Tool::BoxErase:
                break;
        }
    }

    void TileMapEditorPanel::ApplyBoxTool(int minTileX, int minTileY, int maxTileX, int maxTileY)
    {
        if (!m_MapData)
            return;

        if (minTileX > maxTileX)
            std::swap(minTileX, maxTileX);
        if (minTileY > maxTileY)
            std::swap(minTileY, maxTileY);

        const uint16_t replacementTile =
                m_CurrentTool == Tool::BoxFill ? m_SelectedTileID : static_cast<uint16_t>(0);

        for (int32_t tileY = minTileY; tileY <= maxTileY; ++tileY)
        {
            for (int32_t tileX = minTileX; tileX <= maxTileX; ++tileX)
                m_MapData->SetTile(tileX, tileY, replacementTile);
        }
    }

    void TileMapEditorPanel::SaveTileMap()
    {
        if (!m_MapData || m_TileMapHandle == 0 || !Project::GetActive())
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(m_TileMapHandle);
        if (iterator != registry.end())
        {
            auto fullPath = Project::GetAssetFileSystemPath(iterator->second.FilePath);
            TileMapDataSerializer::Serialize(fullPath, m_MapData);
            HIMII_CORE_INFO("TileMap saved to: {0}", fullPath.string());
        }
    }

    void TileMapEditorPanel::SaveTileSet()
    {
        if (!m_TileSet || !m_MapData || !Project::GetActive())
            return;

        const AssetHandle tileSetHandle = m_MapData->GetTileSetHandle();
        if (tileSetHandle == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(tileSetHandle);
        if (iterator != registry.end())
        {
            auto fullPath = Project::GetAssetFileSystemPath(iterator->second.FilePath);
            TileSetSerializer::Serialize(fullPath, m_TileSet);
            HIMII_CORE_INFO("TileSet saved: {0}", fullPath.string());
        }
    }

    bool TileMapEditorPanel::SaveActiveTileMapAssets()
    {
        if (!HasActiveTileMap())
            return false;

        SaveTileMap();
        SaveTileSet();
        return true;
    }

} // namespace Himii
