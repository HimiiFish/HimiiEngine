#include "TextureInspectorPanel.h"
#include "InspectorControls.h"

#include "Resource/AssetManager.h"
#include "Resource/Sprite.h"
#include "Resource/SpriteSheetUtility.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace Himii
{

    bool TextureInspectorPanel::UsesSliceWorkflow() const
    {
        return m_SpriteModeSelection == static_cast<int>(TextureSpriteMode::Multiple);
    }

    void TextureInspectorPanel::SetTextureHandle(AssetHandle textureHandle)
    {
        m_TextureHandle = textureHandle;
        m_PreviewTexture = nullptr;
        m_SelectedSpriteIndex = -1;
        ReloadImportData();
    }

    void TextureInspectorPanel::ReloadImportData()
    {
        if (m_TextureHandle == 0 || !Project::GetActive())
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        assetManager->EnsureDefaultTextureMeta(m_TextureHandle);
        Ref<Asset> asset = assetManager->GetAsset(m_TextureHandle);
        if (asset)
            m_PreviewTexture = std::static_pointer_cast<Texture2D>(asset);

        const TextureImportData* importData = assetManager->GetTextureImportData(m_TextureHandle);
        if (!importData)
            return;

        m_GridCellSize = importData->GridCellSize;
        m_GridOffset = importData->GridOffset;
        m_GridPadding = importData->GridPadding;
        m_PixelsPerUnit = importData->PixelsPerUnit;
        m_SpriteModeSelection = static_cast<int>(importData->SpriteMode);

        if (!UsesSliceWorkflow() || importData->Sprites.empty())
        {
            m_SelectedSpriteIndex = -1;
            m_SpriteNameEditBuffer[0] = '\0';
            return;
        }

        if (m_SelectedSpriteIndex < 0
            || m_SelectedSpriteIndex >= static_cast<int>(importData->Sprites.size()))
        {
            m_SelectedSpriteIndex = 0;
        }

        std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s",
                      importData->Sprites[m_SelectedSpriteIndex].Name.c_str());
    }

    void TextureInspectorPanel::SyncUIToImportData()
    {
        if (m_TextureHandle == 0 || !Project::GetActive())
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);
        importData.GridCellSize = m_GridCellSize;
        importData.GridOffset = m_GridOffset;
        importData.GridPadding = m_GridPadding;
        importData.PixelsPerUnit = m_PixelsPerUnit > 0 ? m_PixelsPerUnit : 100;
        importData.SpriteMode = static_cast<TextureSpriteMode>(m_SpriteModeSelection);
    }

    void TextureInspectorPanel::SyncPendingEditsToMemory()
    {
        SyncUIToImportData();

        if (!UsesSliceWorkflow() || !Project::GetActive())
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);
        if (m_SelectedSpriteIndex >= 0
            && m_SelectedSpriteIndex < static_cast<int>(importData.Sprites.size()))
        {
            importData.Sprites[m_SelectedSpriteIndex].Name = m_SpriteNameEditBuffer;
        }
    }

    bool TextureInspectorPanel::SaveActiveTextureMeta()
    {
        if (m_TextureHandle == 0 || !Project::GetActive())
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        SyncPendingEditsToMemory();
        return assetManager->SaveTextureImportData(m_TextureHandle);
    }

    void TextureInspectorPanel::AddNewSprite()
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !m_PreviewTexture)
            return;

        TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);
        SpriteDefinition newSprite;
        newSprite.Handle = AssetHandle();
        newSprite.Name = "sprite_" + std::to_string(importData.Sprites.size());
        newSprite.PixelRect = {
            0, 0,
            static_cast<int>(m_PreviewTexture->GetWidth()),
            static_cast<int>(m_PreviewTexture->GetHeight())
        };
        importData.Sprites.push_back(newSprite);
        importData.SpriteMode = TextureSpriteMode::Multiple;
        m_SelectedSpriteIndex = static_cast<int>(importData.Sprites.size()) - 1;
        std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s", newSprite.Name.c_str());
    }

    void TextureInspectorPanel::DeleteSelectedSprite()
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);
        if (m_SelectedSpriteIndex < 0
            || m_SelectedSpriteIndex >= static_cast<int>(importData.Sprites.size()))
        {
            return;
        }

        importData.Sprites.erase(importData.Sprites.begin() + m_SelectedSpriteIndex);
        if (importData.Sprites.empty())
        {
            m_SelectedSpriteIndex = -1;
            m_SpriteNameEditBuffer[0] = '\0';
        }
        else
        {
            m_SelectedSpriteIndex = std::min(m_SelectedSpriteIndex,
                                             static_cast<int>(importData.Sprites.size()) - 1);
            std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s",
                          importData.Sprites[m_SelectedSpriteIndex].Name.c_str());
        }
    }

    int TextureInspectorPanel::FindSpriteIndexAtPixel(const glm::ivec2& pixelCoordinate) const
    {
        if (!UsesSliceWorkflow() || !Project::GetActive())
            return -1;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return -1;

        const std::vector<SpriteDefinition>& sprites = assetManager->GetSpritesForTexture(m_TextureHandle);
        for (int spriteIndex = static_cast<int>(sprites.size()) - 1; spriteIndex >= 0; --spriteIndex)
        {
            const SpriteDefinition& sprite = sprites[spriteIndex];
            const int right = sprite.PixelRect.x + sprite.PixelRect.z;
            const int bottom = sprite.PixelRect.y + sprite.PixelRect.w;
            if (pixelCoordinate.x >= sprite.PixelRect.x && pixelCoordinate.x < right
                && pixelCoordinate.y >= sprite.PixelRect.y && pixelCoordinate.y < bottom)
            {
                return spriteIndex;
            }
        }

        return -1;
    }

    void TextureInspectorPanel::OnImGuiRender(bool& isOpen)
    {
        if (!isOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(960.0f, 600.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("Texture Inspector", &isOpen);

        if (m_TextureHandle == 0)
        {
            ImGui::TextWrapped(
                "用于编辑纹理导入设置。在内容浏览器中双击 PNG/JPG 打开；修改后使用 Ctrl+S 保存。");
            ImGui::End();
            return;
        }

        if (ImGui::BeginTable("##TextureInspectorSplit", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
                                  | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, LeftPanelWidth);
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            {
                const ImVec2 leftPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##TextureInspectorLeft", leftPanelSize, false,
                                      ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    DrawLeftInspectorPanel();
                }
                ImGui::EndChild();
            }

            ImGui::TableNextColumn();
            {
                const ImVec2 rightPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##TextureInspectorRight", rightPanelSize, true,
                                      ImGuiWindowFlags_NoScrollbar))
                {
                    DrawRightPreviewPanel();
                }
                ImGui::EndChild();
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void TextureInspectorPanel::DrawLeftInspectorPanel()
    {
        DrawImportSettings();

        if (UsesSliceWorkflow())
            DrawSliceSettings();
    }

    void TextureInspectorPanel::DrawImportSettings()
    {
        DrawInspectorSectionHeader("Import Settings");

        const char* spriteModeLabels[] = {"None", "Single", "Multiple"};
        DrawEnumComboControl("Sprite Mode", m_SpriteModeSelection, spriteModeLabels, 3,
                             [&](int)
                             {
                                 SyncUIToImportData();
                                 if (!UsesSliceWorkflow())
                                     m_SelectedSpriteIndex = -1;
                                 ReloadImportData();
                             });

        DrawPropertyRow("Pixels Per Unit", [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            ImGui::DragScalar("##PixelsPerUnit", ImGuiDataType_U32, &m_PixelsPerUnit, 1.0f,
                              nullptr, nullptr, "%u");
            ImGui::PopItemWidth();
            if (ImGui::IsItemDeactivatedAfterEdit())
                SyncUIToImportData();
        });
    }

    void TextureInspectorPanel::DrawSliceSettings()
    {
        DrawInspectorSectionHeader("Grid Slice");

        DrawIVec2Control("Cell Size", m_GridCellSize, 1.0f, 1, 4096);
        DrawIVec2Control("Offset", m_GridOffset, 1.0f, 0, 4096);
        DrawIVec2Control("Padding", m_GridPadding, 1.0f, 0, 256);

        DrawActionButtonRow("Grid", [&]()
        {
            if (ImGui::Button("Apply Grid Slice", ImVec2(-1.0f, 0.0f)))
            {
                auto assetManager = ResourceSystem::GetAssetManager();
                if (assetManager)
                {
                    SyncUIToImportData();
                    assetManager->ApplyGridSliceToTexture(m_TextureHandle);
                    m_SelectedSpriteIndex = 0;
                    ReloadImportData();
                }
            }
        });

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);

        DrawInspectorSectionHeader("Slice");

        DrawActionButtonRow("Slice Tools", [&]()
        {
            if (ImGui::Button("Add Sprite"))
                AddNewSprite();
            ImGui::SameLine();
            const bool canDelete = m_SelectedSpriteIndex >= 0
                && m_SelectedSpriteIndex < static_cast<int>(importData.Sprites.size());
            if (!canDelete)
                ImGui::BeginDisabled();
            if (ImGui::Button("Delete Sprite"))
                DeleteSelectedSprite();
            if (!canDelete)
                ImGui::EndDisabled();
        });

        if (importData.Sprites.empty())
        {
            ImGui::TextDisabled("尚无切片，请 Apply Grid Slice 或 Add Sprite。");
            return;
        }

        if (m_SelectedSpriteIndex < 0
            || m_SelectedSpriteIndex >= static_cast<int>(importData.Sprites.size()))
        {
            ImGui::TextDisabled("未选中 — 请点击右侧预览中的切片。");
            return;
        }

        SpriteDefinition& sprite = importData.Sprites[m_SelectedSpriteIndex];
        std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s", sprite.Name.c_str());

        DrawSelectedSpriteThumbnail(sprite);

        const auto syncSpriteToMemory = [this]() { SyncPendingEditsToMemory(); };

        DrawInputTextControl("Name", m_SpriteNameEditBuffer, static_cast<int>(sizeof(m_SpriteNameEditBuffer)),
                             syncSpriteToMemory);

        DrawIVec4Control("Pixel Rect", sprite.PixelRect, 1.0f, 0, 8192, syncSpriteToMemory);
        DrawVec2Control("Pivot", sprite.Pivot, 0.01f, 0.0f, 1.0f, syncSpriteToMemory);
    }

    void TextureInspectorPanel::DrawSelectedSpriteThumbnail(const SpriteDefinition& sprite)
    {
        if (!m_PreviewTexture)
            return;

        const float thumbnailSize = 72.0f;
        DrawPropertyRow("Thumbnail", [&]()
        {
            const float aspect = sprite.PixelRect.z > 0 && sprite.PixelRect.w > 0
                ? static_cast<float>(sprite.PixelRect.z) / static_cast<float>(sprite.PixelRect.w)
                : 1.0f;

            float displayWidth = thumbnailSize;
            float displayHeight = thumbnailSize;
            if (aspect >= 1.0f)
                displayHeight = displayWidth / aspect;
            else
                displayWidth = displayHeight * aspect;

            DrawEditorTextureImageSubRect(
                m_PreviewTexture->GetRendererID(), ImVec2(displayWidth, displayHeight),
                sprite.PixelRect, m_PreviewTexture->GetWidth(), m_PreviewTexture->GetHeight());
        });
    }

    void TextureInspectorPanel::DrawRightPreviewPanel()
    {
        if (!m_PreviewTexture)
        {
            ImGui::TextDisabled("无法加载预览纹理。");
            return;
        }

        if (!UsesSliceWorkflow())
        {
            const float previewWidth = ImGui::GetContentRegionAvail().x;
            const float aspectRatio = static_cast<float>(m_PreviewTexture->GetWidth())
                / static_cast<float>(m_PreviewTexture->GetHeight());
            const float previewHeight = previewWidth / aspectRatio;
            DrawEditorTextureImageFull(m_PreviewTexture->GetRendererID(),
                                       ImVec2(previewWidth, previewHeight));
            return;
        }

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        const float previewWidth = ImGui::GetContentRegionAvail().x;
        const float previewHeight = ImGui::GetContentRegionAvail().y;
        const float textureAspect = static_cast<float>(m_PreviewTexture->GetWidth())
            / static_cast<float>(m_PreviewTexture->GetHeight());
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
        DrawEditorTextureImageFull(m_PreviewTexture->GetRendererID(),
                                   ImVec2(displayWidth, displayHeight));

        const std::vector<SpriteDefinition>& sprites = assetManager->GetSpritesForTexture(m_TextureHandle);
        const float scaleX = displayWidth / static_cast<float>(m_PreviewTexture->GetWidth());
        const float scaleY = displayHeight / static_cast<float>(m_PreviewTexture->GetHeight());

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (int spriteIndex = 0; spriteIndex < static_cast<int>(sprites.size()); ++spriteIndex)
        {
            const SpriteDefinition& sprite = sprites[spriteIndex];
            const ImVec2 rectMin(
                imagePosition.x + sprite.PixelRect.x * scaleX,
                imagePosition.y + sprite.PixelRect.y * scaleY);
            const ImVec2 rectMax(
                rectMin.x + sprite.PixelRect.z * scaleX,
                rectMin.y + sprite.PixelRect.w * scaleY);

            const ImU32 fillColor = (spriteIndex == m_SelectedSpriteIndex)
                ? IM_COL32(255, 220, 0, 40)
                : IM_COL32(0, 200, 255, 25);
            drawList->AddRectFilled(rectMin, rectMax, fillColor);

            const ImU32 borderColor = (spriteIndex == m_SelectedSpriteIndex)
                ? IM_COL32(255, 220, 0, 255)
                : IM_COL32(0, 200, 255, 180);
            drawList->AddRect(rectMin, rectMax, borderColor, 0.0f, 0, 2.0f);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const ImVec2 mousePosition = ImGui::GetMousePos();
            const glm::ivec2 clickPixel(
                static_cast<int>((mousePosition.x - imagePosition.x) / scaleX),
                static_cast<int>((mousePosition.y - imagePosition.y) / scaleY));

            const int hitSpriteIndex = FindSpriteIndexAtPixel(clickPixel);
            if (hitSpriteIndex >= 0)
            {
                m_SelectedSpriteIndex = hitSpriteIndex;
                const std::vector<SpriteDefinition>& hitSprites =
                    assetManager->GetSpritesForTexture(m_TextureHandle);
                std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s",
                              hitSprites[hitSpriteIndex].Name.c_str());
                m_IsDrawingSpriteRect = false;
            }
            else
            {
                m_IsDrawingSpriteRect = true;
                m_DrawStartPixel = clickPixel;
                m_DrawEndPixel = clickPixel;
            }
        }

        if (m_IsDrawingSpriteRect && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 mousePosition = ImGui::GetMousePos();
            m_DrawEndPixel.x = static_cast<int>((mousePosition.x - imagePosition.x) / scaleX);
            m_DrawEndPixel.y = static_cast<int>((mousePosition.y - imagePosition.y) / scaleY);

            const ImVec2 dragMin(
                imagePosition.x + std::min(m_DrawStartPixel.x, m_DrawEndPixel.x) * scaleX,
                imagePosition.y + std::min(m_DrawStartPixel.y, m_DrawEndPixel.y) * scaleY);
            const ImVec2 dragMax(
                imagePosition.x + std::max(m_DrawStartPixel.x, m_DrawEndPixel.x) * scaleX,
                imagePosition.y + std::max(m_DrawStartPixel.y, m_DrawEndPixel.y) * scaleY);
            drawList->AddRectFilled(dragMin, dragMax, IM_COL32(255, 80, 80, 50));
            drawList->AddRect(dragMin, dragMax, IM_COL32(255, 80, 80, 255), 0.0f, 0, 2.0f);
        }

        if (m_IsDrawingSpriteRect && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_IsDrawingSpriteRect = false;

            const int left = std::clamp(std::min(m_DrawStartPixel.x, m_DrawEndPixel.x), 0,
                                        static_cast<int>(m_PreviewTexture->GetWidth()));
            const int top = std::clamp(std::min(m_DrawStartPixel.y, m_DrawEndPixel.y), 0,
                                       static_cast<int>(m_PreviewTexture->GetHeight()));
            const int right = std::clamp(std::max(m_DrawStartPixel.x, m_DrawEndPixel.x), 0,
                                         static_cast<int>(m_PreviewTexture->GetWidth()));
            const int bottom = std::clamp(std::max(m_DrawStartPixel.y, m_DrawEndPixel.y), 0,
                                          static_cast<int>(m_PreviewTexture->GetHeight()));

            const int width = right - left;
            const int height = bottom - top;
            if (width > 0 && height > 0)
            {
                TextureImportData& importData = assetManager->GetOrCreateTextureImportData(m_TextureHandle);
                if (m_SelectedSpriteIndex >= 0
                    && m_SelectedSpriteIndex < static_cast<int>(importData.Sprites.size()))
                {
                    importData.Sprites[m_SelectedSpriteIndex].PixelRect = {left, top, width, height};
                }
                else
                {
                    SpriteDefinition newSprite;
                    newSprite.Handle = AssetHandle();
                    newSprite.Name = "sprite_" + std::to_string(importData.Sprites.size());
                    newSprite.PixelRect = {left, top, width, height};
                    importData.Sprites.push_back(newSprite);
                    m_SelectedSpriteIndex = static_cast<int>(importData.Sprites.size()) - 1;
                    std::snprintf(m_SpriteNameEditBuffer, sizeof(m_SpriteNameEditBuffer), "%s",
                                  newSprite.Name.c_str());
                }
                importData.SpriteMode = TextureSpriteMode::Multiple;
            }
        }
    }

} // namespace Himii
