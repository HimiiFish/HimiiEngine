#pragma once
#include "Engine.h"
#include "panel/SceneHierarchyPanel.h"
#include "panel/ContentBrowserPanel.h"
#include "panel/AnimationEditorPanel.h"
#include "panel/TileMapEditorPanel.h"
#include "panel/ParticleEmitterEditorPanel.h"
#include "panel/ScriptConsolePanel.h"
#include "panel/ConsolePanel.h"
#include "panel/EditorPreferencesPanel.h"
#include "panel/ProjectSettingsPanel.h"
#include "panel/TextureInspectorPanel.h"
#include "panel/MaterialEditorPanel.h"
#include "panel/FontDiagnosticsPanel.h"

#include "EngineCore/Core/FileWatcher.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"
#include "Module/Render/Renderer/EditorCamera.h"
#include "commands/EditorCommandHistory.h"
#include "World/World.h"

namespace Himii
{
    class EditorLayer : public Himii::Layer {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;

        virtual void OnUpdate(Himii::Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Himii::Event &event) override;

    private:
        bool OnKeyPressed(KeyPressedEvent &e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

        void OnOverlayRender();
        /// 选中态视口 Gizmo：Light 图标 + 方向箭头、Camera 图标 + 视锥线框。
        void DrawSelectionGizmos(Entity selectedEntity);
        void DrawLightGizmo(Entity lightEntity);
        void DrawCameraGizmo(Entity cameraEntity);
        void DrawBillboardIcon(const Ref<Texture2D> &iconTexture, const glm::vec3 &worldPosition,
                               float worldSize, const glm::vec4 &tintColor);
        void HandleTilemapScenePaint(bool allowPainting);
        void UpdateTilemapPaintSession();
        void DrawTilemapEditOverlay();
        void DrawTilemapGhostPreviewInViewport();
        void DrawTilemapBoxSelectionOverlay(Entity selectedEntity, float cellSize);
        void ResetTilemapBoxSelection();
        void UpdateTilemapHoverFromInput();
        bool TryGetTilemapPaintContext(Entity &outEntity, Ref<TileMapData> &outMapData,
                                       TransformComponent const *&outTransform);
        bool IsTilemapEditContextActive();
        bool IsTilemapPaintActive();
        bool IsTilemapMoveEntityActive();

        void CreateProject(const std::filesystem::path& projectPath, bool is2D = true);
        void OpenProject(const std::filesystem::path &path);
        void SaveProject();
        void BuildProject();
        /// Build Project 前：停止 Play、落盘脏资产、确保 StartScene / AssetRegistry 在磁盘上。
        bool FlushProjectStateForBuild(std::string& errorMessage);
        void UpdateBuildPipelineSession();
        void DrawBuildProgressModal();
        void OpenBuildOutputDirectory(const std::filesystem::path& directoryPath);
        bool IsBuildPipelineBusy() const;

        void OpenCSProject();

        void NewScene();
        void OpenScene();
        void OpenScene(const std::filesystem::path& path);
        void SaveScene();
        void SaveSceneAs();

        void SerializeScene(Ref<Scene> scene, const std::filesystem::path &path);

        void OnScenePlay();
        void OnSceneSimulate();
        void OnSceneStop();

        void OnDuplicateEntity();
        void CompileAndReloadScripts();
        void RequestScriptCompile();
        void StartScenePlay();
        bool IsScriptAssemblyUpToDate() const;
        void MarkScriptsDirtyFromWatcher();
        void SetupScriptFileWatchers();
        void SetupShaderFileWatchers();
        void ReloadShaderAssetsFromWatcher();

        //UI panel
        void UI_Toolbar();
        void DrawMainMenuBar();
        void UpdateMainWindowTitle();
        void RequestQuitApplication();
        void OpenProjectInternal(const std::filesystem::path &path);
        void DrawUnsavedMaterialsModal();
        bool OnWindowCloseRequested(class WindowCloseEvent &event);
        void UpdateEditorCameraForActiveProject();
        void RefreshEditorSkyboxForActiveProject();
        /// 将 Game/ViewPort 同组 Dock 标签前台设为 ViewPort（覆盖 layout ini 的 Selected）。
        bool TrySelectSceneViewportTab();
        void DrawStartupSplash();
        void AdvanceEditorStartupLoading();
        void ApplySplashWindowSize();
        void TransitionToProjectHubWindow();
        void TransitionToEditorWindow();

        enum class EditorStartupState {
            Splash = 0,
            Ready = 1
        };

        enum class EditorStartupLoadingStep {
            ToolbarIcons = 0,
            Skybox,
            Fonts,
            Scene,
            Panels,
            ProjectData,
            Complete
        };

        enum class BuildPipelineUiState {
            Idle = 0,
            Running,
            Succeeded,
            Failed
        };

        static constexpr float MinimumSplashDisplaySeconds = 1.2f;
        static constexpr uint32_t SplashStatusOverlayHeightPixels = 56;
        static constexpr uint32_t ProjectHubClientWidth = 1100;
        static constexpr uint32_t ProjectHubClientHeight = 700;

    private:
        Ref<World> m_World;
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;

        std::filesystem::path m_EditorScenePath;
        std::filesystem::path m_CSharpProjectPath;

        Entity m_SquareEntity;
        Entity m_CameraEntity;

        Entity m_HoveredEntity;

        OrthographicCameraController m_CameraController;
        EditorCamera m_EditorCamera;

        bool m_ViewportFocused = false, m_ViewportHovered = false;
        bool m_GameViewportFocused = false, m_GameViewportHovered = false;
        bool m_ShowGameUserInterface = true;
        bool m_RequestSceneViewportFocus = false;
        bool m_RequestGameViewportFocus = false;
        bool m_RestoreSceneViewportAfterPlay = true;

        // Scene 视口用的离屏帧缓冲
        Ref<Framebuffer> m_Framebuffer;
        Ref<Framebuffer> m_GameFramebuffer;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_GameViewportPanelSize = {0.0f, 0.0f};
        glm::vec2 m_GameViewportBounds[2];
        glm::uvec2 m_GameTargetResolution = {1920, 1080};
        bool m_GameViewHasValidPrimaryCamera = false;
        bool m_GameUserInterfacePrimaryButtonWasHeld = false;

        enum class GameResolutionPreset
        {
            FreeAspect = 0,
            FullHighDefinition,
            HighDefinition,
            Custom
        };
        GameResolutionPreset m_GameResolutionPreset = GameResolutionPreset::FullHighDefinition;

        int m_GizmoType = -1;

        bool m_ShowPhysicsColliders = false;
        bool m_ShowGrid = true;

        enum class SceneState {
            Edit = 0,
            Play = 1,
            Simulate = 2
        };
        SceneHierarchyPanel m_SceneHierarchyPanel;
        ContentBrowserPanel m_ContentBrowserPanel;

        AnimationEditorPanel m_AnimationEditorPanel;
        bool m_ShowAnimationEditorPanel = false;

        TextureInspectorPanel m_TextureInspectorPanel;
        bool m_ShowTextureInspector = false;

        MaterialEditorPanel m_MaterialEditorPanel;
        bool m_ShowMaterialEditor = false;

        FontDiagnosticsPanel m_FontDiagnosticsPanel;
        bool m_ShowFontDiagnostics = false;

        TileMapEditorPanel m_TileMapEditorPanel;
        bool m_ShowTileMapEditor = false;

        ParticleEmitterEditorPanel m_ParticleEmitterEditorPanel;
        bool m_ShowParticleEmitterEditor = false;

        ScriptConsolePanel m_ScriptConsolePanel;
        bool m_ShowScriptConsole = true;

        ConsolePanel m_ConsolePanel;
        bool m_ShowConsole = true;

        EditorPreferencesPanel m_EditorPreferencesPanel;
        bool m_ShowEditorPreferences = false;

        ProjectSettingsPanel m_ProjectSettingsPanel;
        bool m_ShowProjectSettings = false;

        FileWatcher m_ScriptFileWatcher;
        FileWatcher m_ScriptProjectFileWatcher;
        FileWatcher m_ShaderFileWatcher;
        bool m_ScriptsDirty = false;
        bool m_NeedsScriptRebuild = false;
        bool m_PendingPlayAfterCompile = false;
        bool m_ShowScriptReloadNotice = false;
        bool m_WasScriptCompiling = false;
        bool m_NotifyReloadAfterCompile = false;

        Ref<Texture2D> m_IconPlay, m_IconStop, m_IconSimulate;
        Ref<Texture2D> m_IconLightGizmo, m_IconCameraGizmo;
        Ref<Texture2D> m_EngineSplashTexture;
        Ref<TextureCube> m_SkyboxTexture;

        EditorStartupState m_EditorStartupState = EditorStartupState::Splash;
        EditorStartupLoadingStep m_StartupLoadingStep = EditorStartupLoadingStep::ToolbarIcons;
        float m_StartupProgress = 0.0f;
        std::string m_StartupStatusMessage;
        float m_StartupSplashElapsedSeconds = 0.0f;

        SceneState m_SceneState = SceneState::Edit;
        std::string m_Clipboard;

        Ref<Font> m_DefaultFont;

        EditorCommandHistory m_CommandHistory;

        bool m_GizmoTransformCaptureActive = false;
        bool m_GizmoCaptureIsUserInterface = false;
        bool m_GizmoCaptureIsUIText = false;
        bool m_ShowSceneUserInterface = true;
        TransformComponent m_GizmoStartTransform;
        RectTransformComponent m_GizmoStartRectTransform;
        float m_GizmoStartFontSize = 48.0f;

        glm::ivec2 m_TilemapHoveredTile{TileMapCoordinateUtility::InvalidTileCoordinate,
                                        TileMapCoordinateUtility::InvalidTileCoordinate};
        glm::ivec2 m_TilemapBoxSelectionStart{TileMapCoordinateUtility::InvalidTileCoordinate,
                                              TileMapCoordinateUtility::InvalidTileCoordinate};
        glm::ivec2 m_TilemapBoxSelectionEnd{TileMapCoordinateUtility::InvalidTileCoordinate,
                                            TileMapCoordinateUtility::InvalidTileCoordinate};
        bool m_TilemapBoxSelecting = false;
        bool m_TilemapViewportCapture = false;
        UUID m_TilemapPaintSessionEntityUUID = 0;
        bool m_TileMapEditorWasVisible = false;

        enum class PendingEditorSessionAction
        {
            None = 0,
            QuitApplication,
            OpenProject
        };

        PendingEditorSessionAction m_PendingEditorSessionAction = PendingEditorSessionAction::None;
        std::filesystem::path m_PendingOpenProjectPath;
        bool m_ShowUnsavedMaterialsModal = false;

    private:
        struct RecentProject
        {
            std::string Name;
            std::string FilePath;
            long long LastOpened;
        };
        std::vector<RecentProject> m_RecentProjects;

        void LoadRecentProjects();
        void SaveRecentProjects();
        void DrawProjectHub();

        BuildPipelineUiState m_BuildPipelineUiState = BuildPipelineUiState::Idle;
        bool m_BuildProgressModalOpenRequested = false;
        std::string m_BuildProgressStageLabel;
        float m_BuildProgressNormalized = 0.0f;
        std::string m_BuildResultMessage;
        std::filesystem::path m_BuildOutputDirectory;
    };
}