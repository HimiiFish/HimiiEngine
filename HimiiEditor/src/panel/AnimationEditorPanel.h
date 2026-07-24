#pragma once

#include "Himii/Core/Core.h"
#include "Himii/Renderer/Texture.h"
#include "Himii/Scene/SpriteAnimation.h"

#include <imgui.h>
#include <filesystem>

namespace Himii
{

    class AnimationEditorPanel {
    public:
        AnimationEditorPanel();

        void OnImGuiRender(bool& isOpen);
        void SetContext(const std::filesystem::path& path);

        bool SaveActiveAnimationAsset();
        bool HasOpenAnimation() const { return m_CurrentAnimation != nullptr; }
        bool HasSaveableAnimationAsset() const;
        /// 已修改但尚未 Save As（无磁盘路径），Build 前无法自动落盘。
        bool HasUnsavedAnimationWithoutDiskPath() const;

    private:
        static constexpr float LeftPanelWidth = 380.0f;

        struct AnimationFramePreview
        {
            Ref<Texture2D> Texture;
            ImVec2 UV0{0.0f, 1.0f};
            ImVec2 UV1{1.0f, 0.0f};
            bool Valid = false;
        };

        void DrawLeftPanel();
        void DrawRightPanel();
        void DrawMenuBar();

        void UI_Overview();
        void UI_AnimationList();
        void UI_PlaybackSettings();
        void UI_AtlasSource();
        void UI_FrameListSource();

        SpriteAnimationClip* GetActiveClipMutable();
        const SpriteAnimationClip* GetActiveClip() const;
        size_t GetActiveClipFrameCount() const;
        void SelectAnimationClip(const std::string& animationName);

        void DrawAtlasPickerRegion();
        void DrawTimelineRegion();

        void CreateNewAnimation();
        void SaveAnimationAs();
        void SaveAnimation();
        void MarkDirty();

        Ref<Texture2D> GetTextureFromHandle(AssetHandle handle) const;
        AnimationFramePreview ResolveFramePreview(int frameIndex) const;
        void DrawFramePreview(int frameIndex, const ImVec2& displaySize);
        void ImportTextureAsAtlas(const std::filesystem::path& assetPath, bool fillAllGridCells);
        void FillAtlasGridAllCells();
        void AppendFrameFromPayload(const ImGuiPayload* payload);
        void ApplyAtlasCellPick(int column, int row);
        void MoveSelectedTimelineFrame(int direction);
        bool IsAtlasCellUsedInTimeline(int column, int row) const;
        glm::ivec2 GetSelectedAtlasCellCoordinates() const;

    private:
        Ref<SpriteAnimation> m_CurrentAnimation;
        std::filesystem::path m_CurrentFilePath;
        std::string m_CurrentAnimationName = SpriteAnimationDefaultClipName;
        char m_NewAnimationNameBuffer[128] = "new_animation";

        bool m_IsPlaying = false;
        float m_Timer = 0.0f;
        int m_PreviewFrameIndex = 0;
        float m_PreviewFrameRate = 10.0f;

        int m_SelectedFrameIndex = -1;
        uint32_t m_AtlasGridCellSize = 16;
        bool m_AtlasPickReplacesSelectedFrame = false;
        bool m_IsDirty = false;
    };

} // namespace Himii
