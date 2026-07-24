#include "AnimationEditorPanel.h"
#include "InspectorControls.h"

#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Asset/Sprite.h"
#include "Himii/Asset/SpriteSheetUtility.h"
#include "Himii/Project/Project.h"
#include "Himii/Core/Log.h"
#include "Himii/Scene/SpriteAnimationUtility.h"
#include "Himii/Utils/PlatformUtils.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <filesystem>

namespace Himii
{

    AnimationEditorPanel::AnimationEditorPanel() = default;

    void AnimationEditorPanel::OnImGuiRender(bool& isOpen)
    {
        if (!isOpen)
            return;

        ImGui::SetNextWindowSize(ImVec2(960.0f, 600.0f), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Animation Editor", &isOpen, ImGuiWindowFlags_MenuBar))
        {
            ImGui::End();
            return;
        }

        DrawMenuBar();

        if (!m_CurrentAnimation)
        {
            ImGui::TextWrapped("未加载动画资产。使用 File 菜单或从内容浏览器双击 .anim 打开。");
            ImGui::End();
            return;
        }

        if (ImGui::BeginTable("##AnimationEditorSplit", 2,
                              ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
                                  | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Inspector", ImGuiTableColumnFlags_WidthFixed, LeftPanelWidth);
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            {
                const ImVec2 leftPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##AnimationEditorLeft", leftPanelSize, false,
                                      ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    DrawLeftPanel();
                }
                ImGui::EndChild();
            }

            ImGui::TableNextColumn();
            {
                const ImVec2 rightPanelSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##AnimationEditorRight", rightPanelSize, true))
                {
                    DrawRightPanel();
                }
                ImGui::EndChild();
            }

            ImGui::EndTable();
        }

        if (m_IsPlaying && GetActiveClipFrameCount() > 0)
        {
            m_Timer += ImGui::GetIO().DeltaTime;
            const float frameDuration = 1.0f / (m_PreviewFrameRate > 0.0f ? m_PreviewFrameRate : 10.0f);
            if (m_Timer >= frameDuration)
            {
                m_Timer = 0.0f;
                const int frameCount = static_cast<int>(GetActiveClipFrameCount());
                m_PreviewFrameIndex = (m_PreviewFrameIndex + 1) % frameCount;
            }
        }

        ImGui::End();
    }

    void AnimationEditorPanel::DrawMenuBar()
    {
        if (!ImGui::BeginMenuBar())
            return;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Animation"))
            {
                CreateNewAnimation();
                MarkDirty();
            }

            if (ImGui::MenuItem("Open Animation..."))
            {
                const std::string filepath =
                        FileDialog::OpenFile("Sprite Animation (*.anim)\0*.anim\0");
                if (!filepath.empty())
                    SetContext(filepath);
            }

            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveAnimation();

            if (ImGui::MenuItem("Save As..."))
                SaveAnimationAs();

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    void AnimationEditorPanel::DrawLeftPanel()
    {
        UI_Overview();
        UI_AnimationList();
        UI_PlaybackSettings();
        UI_AtlasSource();
        UI_FrameListSource();

        DrawInspectorSectionHeader("Save", "Ctrl+S 会保存工程并写入 .anim 资产。");
        if (DrawTableFillButton("Save Animation", "序列化并重新导入 AssetManager。"))
            SaveAnimation();
    }

    void AnimationEditorPanel::UI_Overview()
    {
        DrawInspectorSectionHeader("Sprite Frames",
                                 "一个 .anim 资产可包含多条命名动画（Godot SpriteFrames 风格）。");

        std::string fileName = m_CurrentFilePath.empty()
            ? "(unsaved)"
            : m_CurrentFilePath.filename().string();
        DrawReadOnlyTextControl("Asset", fileName.c_str(), "当前编辑的 .anim 文件。");

        DrawReadOnlyTextControl("Editing Animation", m_CurrentAnimationName.c_str(),
                                "左侧列表选中后，图集点选与时间轴仅修改该条动画。");

        char frameCountBuffer[32] = {};
        std::snprintf(frameCountBuffer, sizeof(frameCountBuffer), "%zu", GetActiveClipFrameCount());
        DrawReadOnlyTextControl("Frame Count", frameCountBuffer, "当前命名动画的帧数量。");

        char animationCountBuffer[32] = {};
        std::snprintf(animationCountBuffer, sizeof(animationCountBuffer), "%zu",
                      m_CurrentAnimation->GetNamedAnimations().size());
        DrawReadOnlyTextControl("Animation Count", animationCountBuffer, "资产内命名动画条数。");

        const SpriteAnimationClip* activeClip = GetActiveClip();
        const char* sourceMode = "Empty";
        if (activeClip)
        {
            if (activeClip->UsesAtlasFrames(m_CurrentAnimation->GetAtlasTextureHandle()))
                sourceMode = "Atlas Frames";
            else if (m_CurrentAnimation->UsesAtlasTexture())
                sourceMode = "Atlas (pick cells)";
            else if (activeClip->UsesFrameList(m_CurrentAnimation->GetAtlasTextureHandle()))
                sourceMode = "Frame List";
        }
        DrawReadOnlyTextControl("Source Mode", sourceMode,
                                "已切图集：在右侧点选格子组成播放序列，无需再 Slice。");
    }

    void AnimationEditorPanel::UI_AnimationList()
    {
        DrawInspectorSectionHeader("Animations",
                                 "Add / Rename / Delete / 选中后编辑对应时间轴与图集序列。");

        const std::vector<SpriteAnimationClip>& clips = m_CurrentAnimation->GetNamedAnimations();
        for (size_t clipIndex = 0; clipIndex < clips.size(); ++clipIndex)
        {
            const SpriteAnimationClip& clip = clips[clipIndex];
            const bool isSelected = clip.Name == m_CurrentAnimationName;

            ImGui::PushID(static_cast<int>(clipIndex));
            if (isSelected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.75f, 1.0f));

            if (ImGui::Button(clip.Name.c_str(), ImVec2(-1.0f, 0.0f)))
                SelectAnimationClip(clip.Name);

            if (isSelected)
                ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::Separator();

        ImGui::InputText("New Name", m_NewAnimationNameBuffer, sizeof(m_NewAnimationNameBuffer));

        DrawHorizontalButtonPair("AnimationListActions", [&]()
        {
            if (DrawTableFillButton("Add", "添加一条空命名动画。"))
            {
                std::string newName = m_NewAnimationNameBuffer;
                if (newName.empty())
                    newName = "new_animation";

                int suffix = 1;
                std::string candidateName = newName;
                while (m_CurrentAnimation->HasAnimation(candidateName))
                {
                    candidateName = newName + "_" + std::to_string(suffix);
                    ++suffix;
                }

                m_CurrentAnimation->EnsureClip(candidateName);
                SelectAnimationClip(candidateName);
                MarkDirty();
            }
        }, [&]()
        {
            if (DrawTableFillButton("Delete", "删除当前选中的命名动画。"))
            {
                if (m_CurrentAnimation->GetNamedAnimations().size() <= 1)
                    return;

                m_CurrentAnimation->RemoveClip(m_CurrentAnimationName);
                if (!m_CurrentAnimation->GetNamedAnimations().empty())
                    SelectAnimationClip(m_CurrentAnimation->GetNamedAnimations().front().Name);
                MarkDirty();
            }
        });

        static char renameBuffer[128] = {};
        if (renameBuffer[0] == '\0')
            std::snprintf(renameBuffer, sizeof(renameBuffer), "%s", m_CurrentAnimationName.c_str());

        ImGui::InputText("Rename To", renameBuffer, sizeof(renameBuffer));
        if (DrawTableFillButton("Rename", "重命名当前选中的动画。"))
        {
            const std::string newName = renameBuffer;
            if (!newName.empty() && m_CurrentAnimation->RenameClip(m_CurrentAnimationName, newName))
            {
                SelectAnimationClip(newName);
                MarkDirty();
            }
        }
    }

    void AnimationEditorPanel::UI_PlaybackSettings()
    {
        DrawInspectorSectionHeader("Playback",
                                 "当前命名动画的 FPS 与 Loop；实体组件可覆盖 Frame Rate。");

        SpriteAnimationClip* activeClip = GetActiveClipMutable();
        if (!activeClip)
            return;

        float clipFrameRate = activeClip->FrameRate;
        DrawPropertyRow("FPS", [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            ImGui::DragFloat("##Value", &clipFrameRate, 0.5f, 1.0f, 120.0f);
            ImGui::PopItemWidth();
        }, "该条命名动画的默认帧率。");
        if (clipFrameRate != activeClip->FrameRate)
        {
            activeClip->FrameRate = clipFrameRate > 0.0f ? clipFrameRate : 10.0f;
            m_PreviewFrameRate = activeClip->FrameRate;
            MarkDirty();
        }

        int loopModeIndex = static_cast<int>(activeClip->LoopMode);
        const char* loopModeLabels[] = {"Loop", "Once", "PingPong"};
        DrawEnumComboControl("Loop Mode", loopModeIndex, loopModeLabels, 3,
                             [&](int newIndex)
                             {
                                 activeClip->LoopMode =
                                         static_cast<SpriteAnimationLoopMode>(newIndex);
                                 MarkDirty();
                             });
    }

    void AnimationEditorPanel::UI_AtlasSource()
    {
        DrawInspectorSectionHeader(
                "Atlas Frames",
                "拖入已切好的图集 PNG，设置 Cell Size 后在右侧预览逐格点选，组成播放顺序。");

        DrawPropertyRow("Cell Size (px)", [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::DragScalar("##Value", ImGuiDataType_U32, &m_AtlasGridCellSize, 1.0f,
                                  nullptr, nullptr, "%u"))
            {
                if (m_CurrentAnimation->UsesAtlasTexture())
                    m_CurrentAnimation->SetAtlasGridCellSize(m_AtlasGridCellSize);
                MarkDirty();
            }
            ImGui::PopItemWidth();
        }, "与图集导出时的单格像素尺寸一致（如 16、32）。");

        DrawCheckboxControl("Click Replaces Selected", m_AtlasPickReplacesSelectedFrame);

        if (m_CurrentAnimation->UsesAtlasTexture())
        {
            char atlasInfoBuffer[64] = {};
            std::snprintf(atlasInfoBuffer, sizeof(atlasInfoBuffer), "Handle %llu",
                          static_cast<unsigned long long>(m_CurrentAnimation->GetAtlasTextureHandle()));
            DrawReadOnlyTextControl("Atlas Texture", atlasInfoBuffer);
        }

        DrawActionButtonRow("Atlas", [&]()
        {
            if (DrawTableFillButton("Fill All Cells", "可选：按行列顺序填满时间轴（整图逐格动画）。"))
                FillAtlasGridAllCells();
        });

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
                ImportTextureAsAtlas(path, false);
                MarkDirty();
            }
            ImGui::EndDragDropTarget();
        }
    }

    void AnimationEditorPanel::UI_FrameListSource()
    {
        DrawInspectorSectionHeader(
                "Frame List",
                "拖入 Sprite 或纹理作为独立帧。会清空 Atlas 模式。");

        DrawHorizontalButtonPair("FrameListActions", [&]()
        {
            if (DrawTableFillButton("Delete Frame", "删除时间轴中选中的帧。"))
            {
                if (m_SelectedFrameIndex >= 0)
                {
                    if (SpriteAnimationClip* activeClip = GetActiveClipMutable())
                    {
                        if (activeClip->UsesAtlasFrames(m_CurrentAnimation->GetAtlasTextureHandle()))
                            activeClip->RemoveAtlasFrameAt(m_SelectedFrameIndex);
                        else
                            activeClip->RemoveFrameAt(m_SelectedFrameIndex);
                    }
                    m_SelectedFrameIndex = -1;
                    MarkDirty();
                }
            }
        }, [&]()
        {
            if (DrawTableFillButton("Clear All", "清空当前命名动画的全部帧。"))
            {
                if (SpriteAnimationClip* activeClip = GetActiveClipMutable())
                    activeClip->ClearFrames();
                m_SelectedFrameIndex = -1;
                m_PreviewFrameIndex = 0;
                MarkDirty();
            }
        });

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                AppendFrameFromPayload(payload);
                MarkDirty();
            }
            ImGui::EndDragDropTarget();
        }
    }

    void AnimationEditorPanel::DrawRightPanel()
    {
        if (DrawEditorToggleButton(m_IsPlaying ? "Stop" : "Play", m_IsPlaying,
                                   "按时间轴顺序预览播放。"))
        {
            m_IsPlaying = !m_IsPlaying;
            m_Timer = 0.0f;
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        if (ImGui::DragFloat("Preview FPS", &m_PreviewFrameRate, 0.5f, 1.0f, 120.0f))
        {
            if (SpriteAnimationClip* activeClip = GetActiveClipMutable())
            {
                activeClip->FrameRate = m_PreviewFrameRate;
                MarkDirty();
            }
        }
        ImGui::PopItemWidth();

        if (!m_IsPlaying && m_SelectedFrameIndex >= 0
            && m_SelectedFrameIndex < static_cast<int>(GetActiveClipFrameCount()))
            m_PreviewFrameIndex = m_SelectedFrameIndex;

        const float totalHeight = ImGui::GetContentRegionAvail().y;
        const float atlasPickerHeight = totalHeight * 0.58f;
        const float timelineHeight = totalHeight - atlasPickerHeight - ImGui::GetStyle().ItemSpacing.y;

        if (ImGui::BeginChild("##AnimationAtlasPicker", ImVec2(0.0f, atlasPickerHeight), true))
            DrawAtlasPickerRegion();
        ImGui::EndChild();

        if (ImGui::BeginChild("##AnimationTimeline", ImVec2(0.0f, timelineHeight), true))
            DrawTimelineRegion();
        ImGui::EndChild();
    }

    void AnimationEditorPanel::DrawAtlasPickerRegion()
    {
        DrawInspectorSectionHeader(
                "Atlas Picker",
                "左键格子：追加到时间轴，或替换当前选中帧（见左侧选项）。格子高亮表示已用于播放序列。");

        auto assetManager = Project::GetAssetManager();
        if (!assetManager || !m_CurrentAnimation)
            return;

        if (!m_CurrentAnimation->UsesAtlasTexture())
        {
            const ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("##AnimationAtlasDropZone", dropZoneSize);
            DrawInspectorTooltipIfHovered("拖入已切好的图集 PNG/JPG。");

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
                    ImportTextureAsAtlas(path, false);
                    MarkDirty();
                }
                ImGui::EndDragDropTarget();
            }
            return;
        }

        Ref<Texture2D> atlasTexture = std::static_pointer_cast<Texture2D>(
                assetManager->GetAsset(m_CurrentAnimation->GetAtlasTextureHandle()));
        if (!atlasTexture)
            return;

        const float previewWidth = ImGui::GetContentRegionAvail().x;
        const float previewHeight = ImGui::GetContentRegionAvail().y;
        const float textureAspect = static_cast<float>(atlasTexture->GetWidth())
            / static_cast<float>(atlasTexture->GetHeight());
        const float panelAspect = previewWidth / std::max(previewHeight, 1.0f);

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

        const uint32_t cellPixelSize =
                m_CurrentAnimation->GetAtlasGridCellSize() > 0
                    ? m_CurrentAnimation->GetAtlasGridCellSize()
                    : m_AtlasGridCellSize;
        const uint32_t columnCount =
                std::max(1u, atlasTexture->GetWidth() / cellPixelSize);
        const uint32_t rowCount =
                std::max(1u, atlasTexture->GetHeight() / cellPixelSize);

        const float scaleX = displayWidth / static_cast<float>(atlasTexture->GetWidth());
        const float scaleY = displayHeight / static_cast<float>(atlasTexture->GetHeight());
        const float cellScreenWidth = static_cast<float>(cellPixelSize) * scaleX;
        const float cellScreenHeight = static_cast<float>(cellPixelSize) * scaleY;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const glm::ivec2 selectedAtlasCell = GetSelectedAtlasCellCoordinates();

        for (uint32_t row = 0; row < rowCount; ++row)
        {
            for (uint32_t column = 0; column < columnCount; ++column)
            {
                const ImVec2 rectMin(
                        imagePosition.x + static_cast<float>(column) * cellScreenWidth,
                        imagePosition.y + static_cast<float>(row) * cellScreenHeight);
                const ImVec2 rectMax(rectMin.x + cellScreenWidth, rectMin.y + cellScreenHeight);

                const bool usedInTimeline = IsAtlasCellUsedInTimeline(static_cast<int>(column),
                                                                      static_cast<int>(row));
                const bool isSelectedCell = selectedAtlasCell.x == static_cast<int>(column)
                    && selectedAtlasCell.y == static_cast<int>(row);

                if (usedInTimeline)
                    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(80, 160, 255, 35));

                if (isSelectedCell)
                {
                    drawList->AddRectFilled(rectMin, rectMax, IM_COL32(255, 220, 0, 50));
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
                    ApplyAtlasCellPick(column, row);
                }
            }
        }

        if (GetActiveClipFrameCount() == 0)
        {
            ImGui::SetCursorScreenPos(imagePosition);
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.35f, 1.0f),
                               "点击图集格子以添加播放帧。");
        }
    }

    void AnimationEditorPanel::DrawTimelineRegion()
    {
        DrawInspectorSectionHeader(
                "Timeline",
                "从左到右为播放顺序。选中帧后可点图集替换，或用左右箭头调整顺序。");

        DrawHorizontalButtonPair("TimelineReorder", [&]()
        {
            if (DrawTableFillButton("Move Left", "将选中帧向左移动一格。"))
                MoveSelectedTimelineFrame(-1);
        }, [&]()
        {
            if (DrawTableFillButton("Move Right", "将选中帧向右移动一格。"))
                MoveSelectedTimelineFrame(1);
        });

        ImGui::BeginChild("FrameList", ImVec2(0.0f, 0.0f), false,
                          ImGuiWindowFlags_HorizontalScrollbar);

        if (m_CurrentAnimation)
        {
            const int frameCount = static_cast<int>(GetActiveClipFrameCount());
            for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex)
            {
                ImGui::PushID(frameIndex);

                const ImVec2 thumbnailSize{64.0f, 64.0f};
                const AnimationFramePreview preview = ResolveFramePreview(frameIndex);
                const bool isSelected = frameIndex == m_SelectedFrameIndex;
                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

                bool clicked = false;
                if (preview.Valid && preview.Texture)
                {
                    if (ImGui::ImageButton("frame",
                                           (ImTextureID)(intptr_t)preview.Texture->GetRendererID(),
                                           thumbnailSize, preview.UV0, preview.UV1))
                        clicked = true;
                }
                else if (ImGui::Button("?", thumbnailSize))
                    clicked = true;

                if (isSelected)
                    ImGui::PopStyleColor();

                if (clicked)
                {
                    m_SelectedFrameIndex = frameIndex;
                    m_PreviewFrameIndex = frameIndex;
                    m_IsPlaying = false;
                }

                ImGui::SameLine();
                ImGui::PopID();
            }
        }

        ImGui::NewLine();
        ImVec2 dropZoneSize = ImGui::GetContentRegionAvail();
        if (dropZoneSize.y < 50.0f)
            dropZoneSize.y = 50.0f;
        ImGui::Dummy(dropZoneSize);
        DrawInspectorTooltipIfHovered("拖入图集 PNG 到上方预览区；或拖入 Sprite 使用帧列表模式。");

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                AppendFrameFromPayload(payload);
                MarkDirty();
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::EndChild();
    }

    void AnimationEditorPanel::ApplyAtlasCellPick(int column, int row)
    {
        if (!m_CurrentAnimation || !m_CurrentAnimation->UsesAtlasTexture())
            return;

        SpriteAnimationClip* activeClip = GetActiveClipMutable();
        if (!activeClip)
            return;

        const glm::ivec2 atlasCoordinates{column, row};
        const AssetHandle atlasTextureHandle = m_CurrentAnimation->GetAtlasTextureHandle();

        if (m_AtlasPickReplacesSelectedFrame && m_SelectedFrameIndex >= 0
            && activeClip->UsesAtlasFrames(atlasTextureHandle)
            && m_SelectedFrameIndex < static_cast<int>(activeClip->GetFrameCount(atlasTextureHandle)))
        {
            activeClip->SetAtlasFrameCoordinatesAt(m_SelectedFrameIndex, atlasCoordinates);
            m_PreviewFrameIndex = m_SelectedFrameIndex;
        }
        else
        {
            activeClip->Frames.clear();
            activeClip->AddAtlasFrame(atlasCoordinates);
            m_SelectedFrameIndex =
                    static_cast<int>(activeClip->GetFrameCount(atlasTextureHandle)) - 1;
            m_PreviewFrameIndex = m_SelectedFrameIndex;
        }

        m_IsPlaying = false;
        MarkDirty();
    }

    void AnimationEditorPanel::MoveSelectedTimelineFrame(int direction)
    {
        if (!m_CurrentAnimation || m_SelectedFrameIndex < 0 || direction == 0)
            return;

        SpriteAnimationClip* activeClip = GetActiveClipMutable();
        if (!activeClip)
            return;

        const AssetHandle atlasTextureHandle = m_CurrentAnimation->GetAtlasTextureHandle();

        if (activeClip->UsesAtlasFrames(atlasTextureHandle))
        {
            activeClip->MoveAtlasFrame(m_SelectedFrameIndex, direction);
            m_SelectedFrameIndex += direction;
            m_PreviewFrameIndex = m_SelectedFrameIndex;
            MarkDirty();
            return;
        }

        if (activeClip->UsesFrameList(atlasTextureHandle))
        {
            std::vector<AssetHandle>& frames = activeClip->Frames;
            const int newIndex = m_SelectedFrameIndex + direction;
            if (newIndex < 0 || newIndex >= static_cast<int>(frames.size()))
                return;
            std::swap(frames[static_cast<size_t>(m_SelectedFrameIndex)],
                      frames[static_cast<size_t>(newIndex)]);
            m_SelectedFrameIndex = newIndex;
            m_PreviewFrameIndex = newIndex;
            MarkDirty();
        }
    }

    bool AnimationEditorPanel::IsAtlasCellUsedInTimeline(int column, int row) const
    {
        const SpriteAnimationClip* activeClip = GetActiveClip();
        if (!activeClip
            || !activeClip->UsesAtlasFrames(m_CurrentAnimation->GetAtlasTextureHandle()))
            return false;

        for (int frameIndex = 0;
             frameIndex < static_cast<int>(activeClip->AtlasFrameCoordinates.size());
             ++frameIndex)
        {
            const glm::ivec2 coordinates = activeClip->GetAtlasFrameCoordinates(frameIndex);
            if (coordinates.x == column && coordinates.y == row)
                return true;
        }

        return false;
    }

    glm::ivec2 AnimationEditorPanel::GetSelectedAtlasCellCoordinates() const
    {
        const SpriteAnimationClip* activeClip = GetActiveClip();
        if (!activeClip
            || !activeClip->UsesAtlasFrames(m_CurrentAnimation->GetAtlasTextureHandle())
            || m_SelectedFrameIndex < 0
            || m_SelectedFrameIndex >= static_cast<int>(activeClip->AtlasFrameCoordinates.size()))
            return {-1, -1};

        return activeClip->GetAtlasFrameCoordinates(m_SelectedFrameIndex);
    }

    void AnimationEditorPanel::CreateNewAnimation()
    {
        m_CurrentAnimation = std::make_shared<SpriteAnimation>();
        m_CurrentAnimation->EnsureClip(SpriteAnimationDefaultClipName);
        m_CurrentAnimationName = SpriteAnimationDefaultClipName;
        m_CurrentFilePath.clear();
        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = -1;
        m_IsPlaying = false;
        m_PreviewFrameRate = 10.0f;
        m_IsDirty = false;
    }

    void AnimationEditorPanel::SetContext(const std::filesystem::path& path)
    {
        Ref<SpriteAnimation> animation = SpriteAnimationSerializer::Deserialize(path);
        if (!animation)
            return;

        m_CurrentAnimation = animation;
        m_CurrentFilePath = path;

        if (m_CurrentAnimation->HasAnimation(SpriteAnimationDefaultClipName))
            m_CurrentAnimationName = SpriteAnimationDefaultClipName;
        else if (const SpriteAnimationClip* primaryClip = m_CurrentAnimation->GetPrimaryClip())
            m_CurrentAnimationName = primaryClip->Name;
        else if (!m_CurrentAnimation->GetNamedAnimations().empty())
            m_CurrentAnimationName = m_CurrentAnimation->GetNamedAnimations().front().Name;
        else
            m_CurrentAnimationName = SpriteAnimationDefaultClipName;

        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = GetActiveClipFrameCount() > 0 ? 0 : -1;
        m_IsPlaying = false;
        if (const SpriteAnimationClip* activeClip = GetActiveClip())
            m_PreviewFrameRate = activeClip->FrameRate;
        if (m_CurrentAnimation->UsesAtlasTexture())
            m_AtlasGridCellSize = m_CurrentAnimation->GetAtlasGridCellSize();
        m_IsDirty = false;
    }

    void AnimationEditorPanel::MarkDirty()
    {
        m_IsDirty = true;
    }

    void AnimationEditorPanel::SaveAnimationAs()
    {
        const std::string filepath = FileDialog::SaveFile("Sprite Animation (*.anim)\0*.anim\0");
        if (!filepath.empty())
        {
            m_CurrentFilePath = filepath;
            SaveAnimation();
        }
    }

    void AnimationEditorPanel::SaveAnimation()
    {
        if (!m_CurrentAnimation)
            return;

        if (m_CurrentFilePath.empty())
        {
            SaveAnimationAs();
            return;
        }

        SpriteAnimationSerializer::Serialize(m_CurrentFilePath, m_CurrentAnimation);

        if (!Project::GetActive())
            return;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;

        std::error_code errorCode;
        const std::filesystem::path assetsDirectory = Project::GetAssetDirectory();
        const std::filesystem::path relativePath =
                std::filesystem::relative(m_CurrentFilePath, assetsDirectory, errorCode);
        if (errorCode)
            return;

        const AssetHandle importedHandle = assetManager->ImportAsset(relativePath);
        if (importedHandle != 0)
            m_CurrentAnimation->Handle = importedHandle;

        m_IsDirty = false;
        HIMII_CORE_INFO("Animation saved: {0}", m_CurrentFilePath.string());
    }

    bool AnimationEditorPanel::HasSaveableAnimationAsset() const
    {
        return m_CurrentAnimation != nullptr
            && !m_CurrentFilePath.empty()
            && m_IsDirty;
    }

    bool AnimationEditorPanel::HasUnsavedAnimationWithoutDiskPath() const
    {
        return m_CurrentAnimation != nullptr
            && m_IsDirty
            && m_CurrentFilePath.empty();
    }

    bool AnimationEditorPanel::SaveActiveAnimationAsset()
    {
        if (!HasSaveableAnimationAsset())
            return false;

        SaveAnimation();
        return true;
    }

    void AnimationEditorPanel::FillAtlasGridAllCells()
    {
        if (!m_CurrentAnimation || m_CurrentAnimation->GetAtlasTextureHandle() == 0)
            return;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return;

        Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(
                assetManager->GetAsset(m_CurrentAnimation->GetAtlasTextureHandle()));
        if (!texture)
            return;

        const uint32_t cellSize = m_AtlasGridCellSize > 0 ? m_AtlasGridCellSize : 16;
        const uint32_t columnCount = std::max(1u, texture->GetWidth() / cellSize);
        const uint32_t rowCount = std::max(1u, texture->GetHeight() / cellSize);
        m_CurrentAnimation->SetAtlasTexture(m_CurrentAnimation->GetAtlasTextureHandle(), cellSize);
        SpriteAnimationClip& activeClip = m_CurrentAnimation->EnsureClip(m_CurrentAnimationName);
        activeClip.GenerateAtlasGridFrames(columnCount, rowCount);
        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = GetActiveClipFrameCount() > 0 ? 0 : -1;
        MarkDirty();
    }

    void AnimationEditorPanel::ImportTextureAsAtlas(const std::filesystem::path& assetPath,
                                                    bool fillAllGridCells)
    {
        auto assetManager = Project::GetAssetManager();
        if (!assetManager || !m_CurrentAnimation)
            return;

        const AssetHandle textureHandle = assetManager->ImportAsset(assetPath);
        if (textureHandle == 0)
            return;

        m_CurrentAnimation->SetAtlasTexture(textureHandle, m_AtlasGridCellSize);
        if (SpriteAnimationClip* activeClip = GetActiveClipMutable())
            activeClip->ClearFrames();
        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = -1;
        m_IsPlaying = false;

        if (fillAllGridCells)
            FillAtlasGridAllCells();
    }

    void AnimationEditorPanel::AppendFrameFromPayload(const ImGuiPayload* payload)
    {
        if (!payload || !m_CurrentAnimation)
            return;

        AssetHandle spriteHandle = 0;
        SpriteAnimationClip* activeClip = GetActiveClipMutable();
        if (!activeClip)
            return;

        if (AssignSpriteFromContentBrowserPayload(payload, spriteHandle))
        {
            m_CurrentAnimation->ClearAtlasMode();
            activeClip->ClearFrames();
            activeClip->AddFrame(spriteHandle);
            m_SelectedFrameIndex = static_cast<int>(activeClip->Frames.size()) - 1;
            m_PreviewFrameIndex = m_SelectedFrameIndex;
            return;
        }

        AssetHandle textureHandle = 0;
        Ref<Texture2D> texture;
        if (AssignTextureFromContentBrowserPayload(payload, texture, textureHandle))
        {
            m_CurrentAnimation->ClearAtlasMode();
            activeClip->ClearFrames();
            activeClip->AddFrame(textureHandle);
            m_SelectedFrameIndex = static_cast<int>(activeClip->Frames.size()) - 1;
            m_PreviewFrameIndex = m_SelectedFrameIndex;
            return;
        }

        const wchar_t* path = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path assetPath(path);
        std::string extension = assetPath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character)
                       {
                           return static_cast<char>(std::tolower(character));
                       });
        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
            ImportTextureAsAtlas(assetPath, false);
    }

    AnimationEditorPanel::AnimationFramePreview AnimationEditorPanel::ResolveFramePreview(
            int frameIndex) const
    {
        AnimationFramePreview preview;
        if (!m_CurrentAnimation)
            return preview;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return preview;

        const SpriteAnimationClip* activeClip = GetActiveClip();
        if (!activeClip)
            return preview;

        const AssetHandle atlasTextureHandle = m_CurrentAnimation->GetAtlasTextureHandle();
        if (activeClip->UsesAtlasFrames(atlasTextureHandle))
        {
            const glm::ivec2 atlasCoordinates = activeClip->GetAtlasFrameCoordinates(frameIndex);
            const uint32_t cellSize = m_CurrentAnimation->GetAtlasGridCellSize();
            Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(
                    assetManager->GetAsset(atlasTextureHandle));
            if (!texture)
                return preview;

            const glm::ivec4 pixelRect =
                    SpriteSheetUtility::AtlasGridCoordsToPixelRect(atlasCoordinates, cellSize);
            const auto corners =
                    SpriteSheetUtility::PixelRectToImGuiImageUv(pixelRect, texture->GetWidth(),
                                                              texture->GetHeight());

            preview.Texture = texture;
            preview.UV0 = ImVec2(corners.TopLeft.x, corners.TopLeft.y);
            preview.UV1 = ImVec2(corners.BottomRight.x, corners.BottomRight.y);
            preview.Valid = true;
            return preview;
        }

        const AssetHandle frameHandle = activeClip->GetFrame(frameIndex, atlasTextureHandle);
        if (assetManager->IsSpriteHandle(frameHandle))
        {
            const SpriteResolved resolved = assetManager->ResolveSprite(frameHandle);
            if (!resolved.IsValid || !resolved.Texture)
                return preview;

            const SpriteDefinition* spriteDefinition =
                    assetManager->GetSpriteDefinition(frameHandle);
            if (spriteDefinition)
            {
                const auto corners = SpriteSheetUtility::PixelRectToImGuiImageUv(
                        spriteDefinition->PixelRect, resolved.Texture->GetWidth(),
                        resolved.Texture->GetHeight());
                preview.Texture = resolved.Texture;
                preview.UV0 = ImVec2(corners.TopLeft.x, corners.TopLeft.y);
                preview.UV1 = ImVec2(corners.BottomRight.x, corners.BottomRight.y);
                preview.Valid = true;
            }
            return preview;
        }

        preview.Texture = GetTextureFromHandle(frameHandle);
        if (preview.Texture)
        {
            const auto corners = SpriteSheetUtility::FullTextureImGuiUvCorners;
            preview.UV0 = ImVec2(corners.TopLeft.x, corners.TopLeft.y);
            preview.UV1 = ImVec2(corners.BottomRight.x, corners.BottomRight.y);
            preview.Valid = true;
        }
        return preview;
    }

    void AnimationEditorPanel::DrawFramePreview(int frameIndex, const ImVec2& displaySize)
    {
        const AnimationFramePreview preview = ResolveFramePreview(frameIndex);
        if (preview.Valid && preview.Texture)
        {
            ImGui::Image((ImTextureID)(intptr_t)preview.Texture->GetRendererID(),
                         displaySize, preview.UV0, preview.UV1);
        }
        else
        {
            ImGui::TextDisabled("Invalid frame %d", frameIndex);
        }
    }

    SpriteAnimationClip* AnimationEditorPanel::GetActiveClipMutable()
    {
        if (!m_CurrentAnimation)
            return nullptr;
        return m_CurrentAnimation->FindClipMutable(m_CurrentAnimationName);
    }

    const SpriteAnimationClip* AnimationEditorPanel::GetActiveClip() const
    {
        if (!m_CurrentAnimation)
            return nullptr;
        return m_CurrentAnimation->FindClip(m_CurrentAnimationName);
    }

    size_t AnimationEditorPanel::GetActiveClipFrameCount() const
    {
        if (!m_CurrentAnimation)
            return 0;
        return m_CurrentAnimation->GetFrameCount(m_CurrentAnimationName);
    }

    void AnimationEditorPanel::SelectAnimationClip(const std::string& animationName)
    {
        if (!m_CurrentAnimation || !m_CurrentAnimation->HasAnimation(animationName))
            return;

        m_CurrentAnimationName = animationName;
        m_PreviewFrameIndex = 0;
        m_SelectedFrameIndex = GetActiveClipFrameCount() > 0 ? 0 : -1;
        m_IsPlaying = false;
        m_Timer = 0.0f;

        if (const SpriteAnimationClip* activeClip = GetActiveClip())
            m_PreviewFrameRate = activeClip->FrameRate;
    }

    Ref<Texture2D> AnimationEditorPanel::GetTextureFromHandle(AssetHandle handle) const
    {
        if (handle == 0)
            return nullptr;

        auto assetManager = Project::GetAssetManager();
        if (!assetManager)
            return nullptr;

        if (assetManager->IsSpriteHandle(handle))
        {
            const SpriteResolved resolved = assetManager->ResolveSprite(handle);
            return resolved.Texture;
        }

        if (assetManager->IsAssetHandleValid(handle))
            return std::static_pointer_cast<Texture2D>(assetManager->GetAsset(handle));

        return nullptr;
    }

} // namespace Himii
