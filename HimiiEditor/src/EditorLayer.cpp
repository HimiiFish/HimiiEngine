#include "EditorLayer.h"
#include "EditorLayoutDefaults.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "EngineCore/Core/ConsoleLog.h"
#include "EngineCore/Core/FileSystem.h"
#include "Module/Script/ScriptEngine.h"
#include "World/Scene/PrefabSerializer.h"
#include "Module/Script/ScriptCompiler.h"
#include "Module/Script/ScriptIDELauncher.h"
#include "EngineCore/Editor/EditorSettings.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "ProjectBuildPipeline.h"

#include "Module/Render/Renderer/Renderer3D.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "EngineCore/Utils/PlatformUtils.h"
#include "ImGuizmo.h"
#include "yaml-cpp/yaml.h"
#include "commands/EditorCommands.h"
#include "EditorExternalFileDrop.h"
#include "TilemapEditorUtility.h"
#include "Resource/SpriteSheetUtility.h"
#include "Module/Render/Renderer/SpriteRendererUtility.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"
#include "Module/Tilemap/TilemapColliderBuilder.h"
#include "EngineCore/ImGui/ImGuiLayer.h"
#include "Module/Render/Renderer/Renderer.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <GLFW/glfw3.h>

#ifdef HIMII_PLATFORM_WINDOWS
#include <Windows.h>
#include <shellapi.h>
#endif

namespace Himii
{
    EditorLayer::EditorLayer() :
        Layer("Example2D"), m_CameraController(1280.0f / 720.0f)
    {
    }

    void EditorLayer::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        m_EditorStartupState = EditorStartupState::Splash;
        m_StartupLoadingStep = EditorStartupLoadingStep::ToolbarIcons;
        m_StartupProgress = 0.0f;
        m_StartupStatusMessage = "正在启动 Himii Editor…";
        m_StartupSplashElapsedSeconds = 0.0f;

        const std::array<std::filesystem::path, 1> splash_image_candidates = {
            "resources/splash/HimiiEngineSplash.png",
        };

        for (const std::filesystem::path &candidate_path : splash_image_candidates)
        {
            if (!FileSystem::Exists(candidate_path.string()))
                continue;

            m_EngineSplashTexture = Texture2D::Create(candidate_path.string());
            if (m_EngineSplashTexture && m_EngineSplashTexture->IsLoaded())
            {
                HIMII_CORE_INFO("Engine splash image loaded: {0} ({1}x{2})", candidate_path.string(),
                                m_EngineSplashTexture->GetWidth(), m_EngineSplashTexture->GetHeight());
                break;
            }

            m_EngineSplashTexture = nullptr;
        }

        if (!m_EngineSplashTexture || !m_EngineSplashTexture->IsLoaded())
            HIMII_CORE_WARNING("Engine splash image could not be loaded from resources/splash/HimiiEngineSplash.png");

        ApplySplashWindowSize();
    }

    void EditorLayer::ApplySplashWindowSize()
    {
        uint32_t image_width = 543;
        uint32_t image_height = 304;

        if (m_EngineSplashTexture && m_EngineSplashTexture->IsLoaded())
        {
            image_width = m_EngineSplashTexture->GetWidth();
            image_height = m_EngineSplashTexture->GetHeight();
        }

        const uint32_t client_width = image_width;
        const uint32_t client_height = image_height;

        Window &window = Application::Get().GetWindow();
        window.SetClientSize(client_width, client_height);
        window.CenterOnScreen();
    }

    void EditorLayer::TransitionToEditorWindow()
    {
        Window &window = Application::Get().GetWindow();
        window.MaximizeForEditor();
        Renderer::OnWindowResize(window.GetFramebufferWidth(), window.GetFramebufferHeight());
    }

    void EditorLayer::AdvanceEditorStartupLoading()
    {
        HIMII_PROFILE_FUNCTION();

        switch (m_StartupLoadingStep)
        {
            case EditorStartupLoadingStep::ToolbarIcons:
            {
                m_StartupStatusMessage = "正在加载编辑器图标…";
                m_IconPlay = Texture2D::Create("resources/icons/play.png");
                m_IconStop = Texture2D::Create("resources/icons/stop.png");
                m_IconSimulate = Texture2D::Create("resources/icons/simulate.png");
                m_StartupProgress = 0.15f;
                m_StartupLoadingStep = EditorStartupLoadingStep::Skybox;
                break;
            }
            case EditorStartupLoadingStep::Skybox:
            {
                m_StartupStatusMessage = "正在加载天空盒…";
                std::vector<std::string> skybox_faces = {
                    "resources/skybox/right.bmp",  "resources/skybox/left.bmp",  "resources/skybox/top.bmp",
                    "resources/skybox/bottom.bmp", "resources/skybox/front.bmp", "resources/skybox/back.bmp"};
                m_SkyboxTexture = TextureCube::Create(skybox_faces);
                m_StartupProgress = 0.30f;
                m_StartupLoadingStep = EditorStartupLoadingStep::Fonts;
                break;
            }
            case EditorStartupLoadingStep::Fonts:
            {
                m_StartupStatusMessage = "正在加载编辑器字体…";
                // Gameplay default font is initialized from project assets after OpenProject (G1).
                if (ImGuiLayer *imgui_layer = Application::Get().GetImGuiLayer())
                    imgui_layer->LoadEditorFonts();
                HIMII_CORE_INFO("Editor UI fonts loaded from engine content.");
                m_StartupProgress = 0.45f;
                m_StartupLoadingStep = EditorStartupLoadingStep::Scene;
                break;
            }
            case EditorStartupLoadingStep::Scene:
            {
                m_StartupStatusMessage = "正在初始化场景…";
                m_World = CreateRef<World>();
                Application::Get().SetCurrentWorld(m_World);
                m_EditorScene = CreateRef<Scene>();
                m_EditorScene->SetSkybox(m_SkyboxTexture);
                m_ActiveScene = m_EditorScene;
                m_World->SetActiveScene(m_ActiveScene);

                FramebufferSpecification framebuffer_specification{1280, 720};
                framebuffer_specification.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::RED_INTEGER,
                                                         FramebufferFormat::Depth};
                m_Framebuffer = Framebuffer::Create(framebuffer_specification);

                FramebufferSpecification gameFramebufferSpecification{
                        m_GameTargetResolution.x, m_GameTargetResolution.y};
                gameFramebufferSpecification.Attachments = {
                        FramebufferFormat::RGBA8, FramebufferFormat::Depth};
                m_GameFramebuffer = Framebuffer::Create(gameFramebufferSpecification);

                m_EditorCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
                m_StartupProgress = 0.60f;
                m_StartupLoadingStep = EditorStartupLoadingStep::Panels;
                break;
            }
            case EditorStartupLoadingStep::Panels:
            {
                m_StartupStatusMessage = "正在初始化面板…";
                m_SceneHierarchyPanel.SetContext(m_ActiveScene);
                m_SceneHierarchyPanel.SetCommandHistory(&m_CommandHistory);

                if (GLFWwindow *window =
                        static_cast<GLFWwindow *>(Application::Get().GetWindow().GetNativeWindow()))
                    EditorExternalFileDrop::Install(window);

                m_ContentBrowserPanel.SetOnScriptChanged([this]() { MarkScriptsDirtyFromWatcher(); });
                m_StartupProgress = 0.75f;
                m_StartupLoadingStep = EditorStartupLoadingStep::ProjectData;
                break;
            }
            case EditorStartupLoadingStep::ProjectData:
            {
                m_StartupStatusMessage = "正在加载项目列表与设置…";
                LoadRecentProjects();
                EditorSettings::Get().Load();

                auto command_line_arguments = Application::Get().GetCommandLineArgs();
                if (command_line_arguments.Count > 1)
                    OpenProject(command_line_arguments.Args[1]);

                UpdateMainWindowTitle();
                m_StartupStatusMessage = "加载完成";
                m_StartupProgress = 1.0f;
                m_StartupLoadingStep = EditorStartupLoadingStep::Complete;
                break;
            }
            case EditorStartupLoadingStep::Complete:
                break;
        }
    }

    void EditorLayer::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        HIMII_PROFILE_FUNCTION();

        if (m_EditorStartupState == EditorStartupState::Splash)
        {
            m_StartupSplashElapsedSeconds += ts.GetSeconds();

            if (m_StartupLoadingStep != EditorStartupLoadingStep::Complete)
                AdvanceEditorStartupLoading();
            else if (m_StartupSplashElapsedSeconds >= MinimumSplashDisplaySeconds)
            {
                m_EditorStartupState = EditorStartupState::Ready;
                TransitionToEditorWindow();
            }

            return;
        }

        UpdateBuildPipelineSession();

        const bool buildPipelineLocksEditor = IsBuildPipelineBusy();
        const bool wasCompiling = m_WasScriptCompiling;
        // Build session already pumps ScriptCompiler::Update while active; still update when idle.
        if (!ProjectBuildPipeline::IsSessionActive())
            ScriptCompiler::Update();

        // Avoid file-watcher-triggered Debug compiles racing the Release packaging compile.
        if (Project::GetActive() && !buildPipelineLocksEditor)
        {
            m_ScriptFileWatcher.Update(ts.GetSeconds());
            m_ScriptProjectFileWatcher.Update(ts.GetSeconds());
        }

        if (wasCompiling && !ScriptCompiler::IsCompiling())
        {
            // Release GameAssembly compile belongs to Build Pipeline — do not clear editor dirty
            // state or queue another Debug compile mid-session.
            if (!buildPipelineLocksEditor)
            {
                if (ScriptCompiler::GetLastExitCode() == 0)
                {
                    m_ScriptFileWatcher.ClearPendingChange();
                    m_ScriptProjectFileWatcher.ClearPendingChange();

                    if (m_NeedsScriptRebuild)
                    {
                        m_NeedsScriptRebuild = false;
                        m_ScriptsDirty = true;
                        RequestScriptCompile();
                    }
                    else
                    {
                        m_ScriptsDirty = false;
                    }
                }

                if (m_NotifyReloadAfterCompile)
                {
                    m_ShowScriptReloadNotice = true;
                    m_NotifyReloadAfterCompile = false;
                }
            }
        }
        m_WasScriptCompiling = ScriptCompiler::IsCompiling();

        if (m_PendingPlayAfterCompile && !ScriptCompiler::IsCompiling())
        {
            m_PendingPlayAfterCompile = false;
            if (ScriptCompiler::GetLastExitCode() == 0)
                StartScenePlay();
        }

        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
            (spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
        {
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
        }

        if (m_GameResolutionPreset == GameResolutionPreset::FreeAspect)
        {
            m_GameTargetResolution.x =
                    std::max(1u, static_cast<uint32_t>(m_GameViewportPanelSize.x));
            m_GameTargetResolution.y =
                    std::max(1u, static_cast<uint32_t>(m_GameViewportPanelSize.y));
        }
        else if (m_GameResolutionPreset == GameResolutionPreset::FullHighDefinition)
        {
            m_GameTargetResolution = {1920, 1080};
        }
        else if (m_GameResolutionPreset == GameResolutionPreset::HighDefinition)
        {
            m_GameTargetResolution = {1280, 720};
        }
        m_ActiveScene->OnViewportResize(m_GameTargetResolution.x, m_GameTargetResolution.y);

        // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
        Renderer2D::ResetStats();

        m_Framebuffer->Bind();

        glm::vec4 clearColor = {0.18f, 0.28f, 0.46f, 1.0f};

        Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
        if (camera)
            clearColor = camera.GetComponent<CameraComponent>().Camera.GetBackgroundColor();
        
        RenderCommand::SetClearColor(clearColor);
        RenderCommand::Clear();

        m_Framebuffer->ClearAttachment(1, -1); // 1号附件清除为 -1（无实体）

        switch (m_SceneState)
        {
            case SceneState::Edit:
            {
                bool is2D = false;
                if (Project::GetActive())
                    is2D = Project::GetActive()->GetConfig().Is2D;

                m_EditorCamera.SetOrthographicProjection(is2D);
                m_EditorCamera.OnUpdate(ts, m_ViewportHovered, is2D);
                
                m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera, m_ShowSceneUserInterface);
                break;
            }
            case SceneState::Play:
            {
                bool isTwoDimensional = false;
                if (Project::GetActive())
                    isTwoDimensional = Project::GetActive()->GetConfig().Is2D;
                m_EditorCamera.SetOrthographicProjection(isTwoDimensional);
                m_EditorCamera.OnUpdate(
                        ts, m_ViewportHovered, isTwoDimensional);
                m_ActiveScene->RenderEditorView(
                        m_EditorCamera, m_ShowSceneUserInterface);
                break;
            }
            case SceneState::Simulate:
                {
                    bool is2D = false;
                    if (Project::GetActive())
                        is2D = Project::GetActive()->GetConfig().Is2D;

                    m_EditorCamera.SetOrthographicProjection(is2D);
                    m_EditorCamera.OnUpdate(ts, m_ViewportHovered, is2D);
                    m_World->OnUpdateSimulation(ts, m_EditorCamera);
                }
                break;
            default:
                break;
        }

        HIMII_PROFILE_SCOPE("Renderer Draw");

        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        my = viewportSize.y - my;
        int mouseX = (int)mx;
        int mouseY = (int)my;

        if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        {
            int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
            if (pixelData != -1 && m_ActiveScene->Registry().valid((entt::entity)pixelData))
            {
                 m_HoveredEntity = Entity((entt::entity)pixelData, m_ActiveScene.get());
            }
            else
            {
                m_HoveredEntity = Entity();
            }
        }

        OnOverlayRender();

        m_Framebuffer->Unbind();

        const FramebufferSpecification gameFramebufferSpecification =
                m_GameFramebuffer->GetSpecification();
        if (gameFramebufferSpecification.Width != m_GameTargetResolution.x
            || gameFramebufferSpecification.Height != m_GameTargetResolution.y)
        {
            m_GameFramebuffer->Resize(
                    m_GameTargetResolution.x, m_GameTargetResolution.y);
        }

        m_GameFramebuffer->Bind();
        Entity primaryCameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
        m_GameViewHasValidPrimaryCamera =
                primaryCameraEntity
                && primaryCameraEntity.HasComponent<TransformComponent>();
        glm::vec4 gameClearColor{0.18f, 0.28f, 0.46f, 1.0f};
        if (m_GameViewHasValidPrimaryCamera)
            gameClearColor =
                    primaryCameraEntity.GetComponent<CameraComponent>().Camera.GetBackgroundColor();
        RenderCommand::SetClearColor(gameClearColor);
        RenderCommand::Clear();

        Scene::UserInterfacePointerFrameInput userInterfacePointerInput{};
        if (m_SceneState == SceneState::Play)
        {
            const bool primaryButtonHeld = Input::IsMouseButtonPressed(Mouse::ButtonLeft);
            userInterfacePointerInput.Enabled =
                    m_GameViewportHovered && m_ShowGameUserInterface && m_GameViewHasValidPrimaryCamera;
            if (userInterfacePointerInput.Enabled)
            {
                const glm::vec2 mousePosition = Input::GetMousePosition();
                const glm::vec2 gameImageSize = m_GameViewportBounds[1] - m_GameViewportBounds[0];
                if (gameImageSize.x > 0.0f && gameImageSize.y > 0.0f
                    && m_GameTargetResolution.x > 0 && m_GameTargetResolution.y > 0)
                {
                    const float normalizedX =
                            (mousePosition.x - m_GameViewportBounds[0].x) / gameImageSize.x;
                    const float normalizedY =
                            (mousePosition.y - m_GameViewportBounds[0].y) / gameImageSize.y;
                    if (normalizedX >= 0.0f && normalizedX <= 1.0f
                        && normalizedY >= 0.0f && normalizedY <= 1.0f)
                    {
                        userInterfacePointerInput.HasPosition = true;
                        userInterfacePointerInput.PositionInTargetPixels = {
                                normalizedX * static_cast<float>(m_GameTargetResolution.x),
                                (1.0f - normalizedY) * static_cast<float>(m_GameTargetResolution.y)};
                    }
                }
            }
            userInterfacePointerInput.PrimaryButtonHeld = primaryButtonHeld;
            userInterfacePointerInput.PrimaryButtonPressedThisFrame =
                    primaryButtonHeld && !m_GameUserInterfacePrimaryButtonWasHeld;
            userInterfacePointerInput.PrimaryButtonReleasedThisFrame =
                    !primaryButtonHeld && m_GameUserInterfacePrimaryButtonWasHeld;
            m_GameUserInterfacePrimaryButtonWasHeld = primaryButtonHeld;
            m_ActiveScene->SetUserInterfacePointerInput(userInterfacePointerInput);
            m_World->OnUpdateRuntime(ts, m_ShowGameUserInterface);
        }
        else if (m_GameViewHasValidPrimaryCamera)
        {
            m_GameUserInterfacePrimaryButtonWasHeld = false;
            m_ActiveScene->SetUserInterfacePointerInput({});
            m_ActiveScene->RenderGameView(
                    m_GameTargetResolution.x, m_GameTargetResolution.y,
                    m_ShowGameUserInterface);
        }
        m_GameFramebuffer->Unbind();
    }
    void EditorLayer::OnImGuiRender()
    {
        HIMII_PROFILE_FUNCTION();

        if (m_EditorStartupState == EditorStartupState::Splash)
        {
            DrawStartupSplash();
            return;
        }

        if (!Project::GetActive())
        {
            DrawProjectHub();
            return;
        }

        static bool dockingEnable = true;

        if (dockingEnable)
        {
            static bool dockspaceOpen = true;
            static bool opt_fullscreen = true;
            static bool opt_padding = false;
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            if (opt_fullscreen)
            {
                const ImGuiViewport *viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            }

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
            ImGui::PopStyleVar();

            if (opt_fullscreen)
                ImGui::PopStyleVar(2);

            ImGuiIO &io = ImGui::GetIO();
            ImGuiStyle &style = ImGui::GetStyle();
            float minWinSizeX = style.WindowMinSize.x;
            style.WindowMinSize.x = 370.0f;

            DrawMainMenuBar();

            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                if (ImGuiLayer *imgui_layer = Application::Get().GetImGuiLayer())
                    imgui_layer->EnableLayoutPersistence();

                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
                EditorLayoutDefaults::ApplyDefaultDockLayoutIfNeeded(dockspace_id);
            }

            style.WindowMinSize.x = minWinSizeX;

            ImGui::End();

            if (m_ShowScriptReloadNotice)
            {
                ImGui::OpenPopup("ScriptReloadNotice");
                m_ShowScriptReloadNotice = false;
            }
            if (ImGui::BeginPopupModal("ScriptReloadNotice", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Scripts reloaded. Press Play again to run.");
                if (ImGui::Button("OK"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            DrawBuildProgressModal();

            m_SceneHierarchyPanel.OnImGuiRender();
            m_ContentBrowserPanel.OnImGuiRender();
            if (m_ShowScriptConsole)
                m_ScriptConsolePanel.OnImGuiRender(&m_ShowScriptConsole);
            if (m_ShowConsole)
                m_ConsolePanel.OnImGuiRender(&m_ShowConsole);
            if (m_ShowFontDiagnostics)
            {
                m_FontDiagnosticsPanel.GetShowFlag() = m_ShowFontDiagnostics;
                m_FontDiagnosticsPanel.SetFont(Font::GetDefault());
                m_FontDiagnosticsPanel.OnImGuiRender();
                m_ShowFontDiagnostics = m_FontDiagnosticsPanel.GetShowFlag();
            }
            if (m_ShowEditorPreferences)
                m_EditorPreferencesPanel.OnImGuiRender(&m_ShowEditorPreferences);
            if (m_ShowProjectSettings)
                m_ProjectSettingsPanel.OnImGuiRender(&m_ShowProjectSettings);
            m_AnimationEditorPanel.OnImGuiRender(m_ShowAnimationEditorPanel);
            m_TextureInspectorPanel.OnImGuiRender(m_ShowTextureInspector);
            m_MaterialEditorPanel.OnImGuiRender(m_ShowMaterialEditor);
            m_TileMapEditorPanel.OnImGuiRender(m_ShowTileMapEditor);
            UpdateTilemapPaintSession();
            m_ParticleEmitterEditorPanel.OnImGuiRender(m_ShowParticleEmitterEditor);

            AssetHandle tmEditorRequest = m_SceneHierarchyPanel.GetTileMapEditorRequest();
            if (tmEditorRequest != 0)
            {
                m_ShowTileMapEditor = true;
                m_TileMapEditorPanel.Open(tmEditorRequest);
            }
            AssetHandle textureInspectorRequest = m_SceneHierarchyPanel.GetTextureInspectorRequest();
            if (textureInspectorRequest == 0)
                textureInspectorRequest = m_ContentBrowserPanel.GetTextureInspectorRequest();
            if (textureInspectorRequest != 0)
            {
                m_ShowTextureInspector = true;
                m_TextureInspectorPanel.SetTextureHandle(textureInspectorRequest);
            }

            std::filesystem::path animationEditorRequest =
                m_SceneHierarchyPanel.GetAnimationEditorRequest();
            if (animationEditorRequest.empty())
                animationEditorRequest = m_ContentBrowserPanel.GetAnimationEditorRequest();
            if (!animationEditorRequest.empty())
            {
                m_ShowAnimationEditorPanel = true;
                m_AnimationEditorPanel.SetContext(animationEditorRequest);
            }

            AssetHandle peEditorRequest = m_SceneHierarchyPanel.GetParticleEmitterEditorRequest();
            if (peEditorRequest != 0)
            {
                m_ShowParticleEmitterEditor = true;
                m_ParticleEmitterEditorPanel.Open(peEditorRequest);
            }

            AssetHandle materialEditorRequest = m_SceneHierarchyPanel.GetMaterialEditorRequest();
            if (materialEditorRequest == 0)
                materialEditorRequest = m_ContentBrowserPanel.GetMaterialEditorRequest();
            if (materialEditorRequest != 0)
            {
                m_ShowMaterialEditor = true;
                m_MaterialEditorPanel.SetMaterialHandle(materialEditorRequest);
            }

            ImGui::Begin("Stats");
            auto stats = Himii::Renderer2D::GetStatistics();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quad Count: %d", stats.QuadCount);
            ImGui::Text("Vertex Count: %d", stats.GetTotalVertexCount());
            ImGui::Text("Index Count: %d", stats.GetTotalIndexCount());

            ImGui::Separator();
            auto stats3D = Himii::Renderer3D::GetStatistics();
            ImGui::Text("Renderer3D Stats:");
            ImGui::Text("Draw Calls: %d", stats3D.DrawCalls);
            ImGui::Text("Vertex Count: %d", stats3D.GetTotalVertexCount());
            ImGui::Text("Index Count: %d", stats3D.GetTotalIndexCount());
            ImGui::Text("Face Count: %d", stats3D.GetTotalIndexCount() / 3);
            ImGui::End();

            ImGui::Begin("Settings");
            ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);

            ImGui::End();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
            ImGui::Begin("ViewPort");
            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();
            m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
            m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();
            Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered || IsBuildPipelineBusy());

            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if (m_ViewportSize != *((glm::vec2 *)&viewportPanelSize))
            {
                m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
            }

            uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            const ImVec2 viewportImageMin = ImGui::GetCursorScreenPos();
            ImGui::Image(reinterpret_cast<void *>(textureID), ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1),
                         ImVec2(1, 0));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const wchar_t *path = (const wchar_t *)payload->Data;
                    std::filesystem::path assetPath(path);
                    const std::filesystem::path fullAssetPath =
                            std::filesystem::path(Project::GetAssetDirectory()) / assetPath;

                    if (assetPath.extension() == ".himii")
                    {
                        OpenScene(fullAssetPath);
                    }
                    else if (assetPath.extension() == ".hprefab" && m_EditorScene)
                    {
                        Entity instantiatedEntity =
                                PrefabSerializer::Instantiate(m_EditorScene, fullAssetPath);
                        if (instantiatedEntity)
                            m_SceneHierarchyPanel.SetSelectedEntity(instantiatedEntity);
                    }
                }

                ImGui::EndDragDropTarget();
            }

            Entity selectEntity = m_SceneHierarchyPanel.GetSelectedEntity();
            const bool isTilemapEditEntity =
                    selectEntity && selectEntity.HasComponent<TilemapComponent>();
            if (isTilemapEditEntity)
            {
                const AssetHandle tileMapHandle =
                        selectEntity.GetComponent<TilemapComponent>().TileMapHandle;
                if (tileMapHandle != 0
                    && m_TileMapEditorPanel.GetTileMapHandle() != tileMapHandle)
                {
                    m_TileMapEditorPanel.Open(tileMapHandle);
                }
            }

            const bool tilemapPaintModeActive = IsTilemapPaintActive();

            if (m_ViewportFocused || m_ViewportHovered)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_H) && isTilemapEditEntity)
                    m_TileMapEditorPanel.ToggleMoveEntityMode();
            }

            // gizmos（Tilemap 绘制模式关闭时才显示，避免与笔刷抢鼠标）
            if (selectEntity && m_GizmoType != -1 && m_SceneState == SceneState::Edit && !tilemapPaintModeActive)
            {
                bool isUI = selectEntity.HasComponent<RectTransformComponent>();
                bool isTransform = selectEntity.HasComponent<TransformComponent>();
                const bool isCanvasRoot = isUI && selectEntity.HasComponent<CanvasComponent>();

                // Canvas 根禁止 Gizmo 移动/缩放；UI 范围由主相机取景框表示。
                if ((isUI || isTransform) && !isCanvasRoot)
                {
                    // UI 强制使用正交投影，3D 物体取决于项目配置
                    ImGuizmo::SetOrthographic(isUI ? true : Project::GetConfig().Is2D);
                    ImGuizmo::SetDrawlist();

                    // 修复 ImGuizmo 矩形区域 (使用你已有的 Viewport Bounds 会更精准)
                    ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                                      m_ViewportBounds[1].x - m_ViewportBounds[0].x,
                                      m_ViewportBounds[1].y - m_ViewportBounds[0].y);

                    // 准备相机矩阵
                    glm::mat4 cameraProjection;
                    glm::mat4 cameraView;
                    glm::mat4 transform;
                    glm::mat4 designToWorld = glm::mat4(1.0f);
                    float userInterfaceVisualScaleX = 1.0f;
                    float userInterfaceVisualScaleY = 1.0f;
                    if (isUI)
                    {
                        cameraProjection = m_EditorCamera.GetProjection();
                        cameraView = m_EditorCamera.GetViewMatrix();
                        designToWorld = m_EditorScene->GetDesignToWorldMatrix();

                        const auto& userInterfaceTransform =
                                selectEntity.GetComponent<RectTransformComponent>();
                        const Scene::ResolvedRectTransform resolvedRectTransform =
                                m_EditorScene->ResolveRectTransform(
                                        selectEntity,
                                        static_cast<float>(m_GameTargetResolution.x),
                                        static_cast<float>(m_GameTargetResolution.y));
                        userInterfaceVisualScaleX = resolvedRectTransform.Size.x;
                        userInterfaceVisualScaleY = resolvedRectTransform.Size.y;
                        transform = designToWorld
                                * m_EditorScene->GetEntityWorldTransformMatrix(selectEntity)
                                * glm::scale(glm::mat4(1.0f),
                                             glm::vec3(userInterfaceVisualScaleX, userInterfaceVisualScaleY, 1.0f));
                    }
                    else
                    {
                        cameraProjection = m_EditorCamera.GetProjection();
                        cameraView = m_EditorCamera.GetViewMatrix();
                        transform = m_EditorScene->GetEntityWorldTransformMatrix(selectEntity);
                    }

                    const bool isSpriteRendererEntity =
                            !isUI && selectEntity.HasComponent<SpriteRendererComponent>()
                            && Project::GetActive() && Project::GetConfig().Is2D;

                    // 决定 Gizmo 的操作类型 (2D/UI 通常只需 XY 移动和缩放)
                    ImGuizmo::OPERATION currentGizmoOperation = (ImGuizmo::OPERATION)m_GizmoType;
                    if (isSpriteRendererEntity)
                    {
                        if (m_GizmoType == ImGuizmo::TRANSLATE)
                            currentGizmoOperation =
                                    (ImGuizmo::OPERATION)(ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y);
                        else if (m_GizmoType == ImGuizmo::ROTATE)
                            currentGizmoOperation = ImGuizmo::ROTATE_Z;
                        else if (m_GizmoType == ImGuizmo::SCALE)
                            currentGizmoOperation =
                                    (ImGuizmo::OPERATION)(ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y);
                    }
                    else if (Project::GetConfig().Is2D || isUI)
                    {
                        if (m_GizmoType == ImGuizmo::TRANSLATE)
                            currentGizmoOperation =
                                    (ImGuizmo::OPERATION)(ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y);
                        else if (m_GizmoType == ImGuizmo::ROTATE)
                            currentGizmoOperation = ImGuizmo::ROTATE_Z;
                        else if (m_GizmoType == ImGuizmo::SCALE)
                            currentGizmoOperation = (ImGuizmo::OPERATION)(ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y);
                    }

                    // 吸附功能保持不变
                    bool snap = Input::IsKeyPressed(Key::LeftControl);
                    float snapValue = 0.5f;
                    if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                        snapValue = 45.0f;
                    float snapValues[3] = {snapValue, snapValue, snapValue};

                    if (ImGuizmo::IsUsing() && !m_GizmoTransformCaptureActive)
                    {
                        m_GizmoTransformCaptureActive = true;
                        m_GizmoCaptureIsUserInterface = isUI;
                        m_GizmoCaptureIsUIText = false;
                        if (isUI)
                            m_GizmoStartRectTransform =
                                    selectEntity.GetComponent<RectTransformComponent>();
                        else
                            m_GizmoStartTransform = selectEntity.GetComponent<TransformComponent>();
                    }

                    // 绘制 Gizmo（Local 空间；矩阵使用世界变换以正确显示层级位置）
                    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                         currentGizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr,
                                         snap ? snapValues : nullptr);

                    // 写回数据
                    if (ImGuizmo::IsUsing())
                    {
                        if (isUI)
                        {
                            const glm::mat4 designMatrixWithVisualScale =
                                    glm::inverse(designToWorld) * transform;

                            Entity parentEntity = m_EditorScene->GetParentEntity(selectEntity);
                            const glm::mat4 parentWorldMatrix =
                                    parentEntity ? m_EditorScene->GetEntityWorldTransformMatrix(parentEntity)
                                                 : glm::mat4(1.0f);
                            const glm::mat4 localMatrixWithVisualScale =
                                    glm::inverse(parentWorldMatrix) * designMatrixWithVisualScale;

                            glm::vec3 translation{};
                            glm::vec3 rotation{};
                            glm::vec3 scale{};
                            Math::DecomposeTransform(localMatrixWithVisualScale, translation, rotation, scale);

                            auto& userInterfaceTransform =
                                    selectEntity.GetComponent<RectTransformComponent>();
                            userInterfaceTransform.RotationRadians = rotation.z;

                            float sizeScaleX = std::abs(scale.x);
                            float sizeScaleY = std::abs(scale.y);
                            glm::vec2 parentRectSize(0.0f);
                            Entity parentRectEntity = m_EditorScene->GetParentEntity(selectEntity);
                            if (parentRectEntity)
                            {
                                const Scene::ResolvedRectTransform parentResolvedRectTransform =
                                        m_EditorScene->ResolveRectTransform(
                                                parentRectEntity,
                                                static_cast<float>(m_GameTargetResolution.x),
                                                static_cast<float>(m_GameTargetResolution.y));
                                parentRectSize = parentResolvedRectTransform.Size;
                            }
                            const glm::vec2 anchorRange =
                                    userInterfaceTransform.AnchorMaximum
                                    - userInterfaceTransform.AnchorMinimum;
                            const glm::vec2 stretchedSize = parentRectSize * anchorRange;
                            const glm::vec2 manipulatedRectSize(
                                    std::max(0.01f, sizeScaleX),
                                    std::max(0.01f, sizeScaleY));
                            userInterfaceTransform.SizeDelta =
                                    manipulatedRectSize - stretchedSize;

                            const glm::vec2 anchorReference =
                                    -parentRectSize * 0.5f
                                    + parentRectSize
                                              * (userInterfaceTransform.AnchorMinimum
                                                 + anchorRange * userInterfaceTransform.Pivot);
                            userInterfaceTransform.AnchoredPosition =
                                    glm::vec2(translation)
                                    - (glm::vec2(0.5f) - userInterfaceTransform.Pivot)
                                              * manipulatedRectSize
                                    - anchorReference;

                            m_EditorScene->NotifyEntityLocalTransformChanged(selectEntity);
                        }
                        else
                        {
                            m_EditorScene->ApplyWorldMatrixAsLocalTransform(selectEntity, transform);
                            m_EditorScene->NotifyEntityLocalTransformChanged(selectEntity);
                        }
                    }
                    else if (m_GizmoTransformCaptureActive)
                    {
                        if (m_SceneState == SceneState::Edit)
                        {
                            if (m_GizmoCaptureIsUserInterface
                                && selectEntity.HasComponent<RectTransformComponent>())
                            {
                                const auto &afterTransform =
                                        selectEntity.GetComponent<RectTransformComponent>();
                                if (m_GizmoStartRectTransform.AnchorMinimum
                                            != afterTransform.AnchorMinimum
                                    || m_GizmoStartRectTransform.AnchorMaximum
                                            != afterTransform.AnchorMaximum
                                    || m_GizmoStartRectTransform.Pivot != afterTransform.Pivot
                                    || m_GizmoStartRectTransform.AnchoredPosition
                                            != afterTransform.AnchoredPosition
                                    || m_GizmoStartRectTransform.RotationRadians
                                            != afterTransform.RotationRadians
                                    || m_GizmoStartRectTransform.SizeDelta
                                            != afterTransform.SizeDelta)
                                {
                                    m_CommandHistory.Execute(CreateScope<ModifyRectTransformCommand>(
                                        m_EditorScene, selectEntity.GetUUID(), m_GizmoStartRectTransform,
                                        afterTransform));
                                }
                            }
                            else if (!m_GizmoCaptureIsUserInterface
                                     && selectEntity.HasComponent<TransformComponent>())
                            {
                                const auto &afterTransform = selectEntity.GetComponent<TransformComponent>();
                                if (m_GizmoStartTransform.Position != afterTransform.Position
                                    || m_GizmoStartTransform.Rotation != afterTransform.Rotation
                                    || m_GizmoStartTransform.Scale != afterTransform.Scale)
                                {
                                    m_CommandHistory.Execute(CreateScope<ModifyTransformCommand>(
                                        m_EditorScene, selectEntity.GetUUID(), m_GizmoStartTransform,
                                        afterTransform));
                                }
                            }
                        }
                        m_GizmoTransformCaptureActive = false;
                        m_GizmoCaptureIsUIText = false;
                    }
                }
            }

            if (tilemapPaintModeActive)
            {
                ImGui::SetCursorScreenPos(viewportImageMin);
                ImGui::InvisibleButton("##viewport_tilemap_paint", ImVec2(m_ViewportSize.x, m_ViewportSize.y),
                                        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
                m_TilemapViewportCapture = ImGui::IsItemHovered() || ImGui::IsItemActive();
                HandleTilemapScenePaint(true);
                DrawTilemapGhostPreviewInViewport();
            }
            else
            {
                m_TilemapViewportCapture = false;
                if (IsTilemapEditContextActive())
                    HandleTilemapScenePaint(false);
            }

            // ImGui::End();
            ImGui::PopStyleVar();

            UI_Toolbar();

            ImGui::End();
        }

        if (m_RequestGameViewportFocus)
            ImGui::SetNextWindowFocus();
        ImGui::Begin("Game");
        m_RequestGameViewportFocus = false;
        const char* resolutionPreview = "1920 x 1080";
        if (m_GameResolutionPreset == GameResolutionPreset::FreeAspect)
            resolutionPreview = "Free Aspect";
        else if (m_GameResolutionPreset == GameResolutionPreset::HighDefinition)
            resolutionPreview = "1280 x 720";
        else if (m_GameResolutionPreset == GameResolutionPreset::Custom)
            resolutionPreview = "Custom";

        ImGui::SetNextItemWidth(150.0f);
        if (ImGui::BeginCombo("Resolution", resolutionPreview))
        {
            if (ImGui::Selectable(
                        "Free Aspect",
                        m_GameResolutionPreset == GameResolutionPreset::FreeAspect))
                m_GameResolutionPreset = GameResolutionPreset::FreeAspect;
            if (ImGui::Selectable(
                        "1920 x 1080",
                        m_GameResolutionPreset == GameResolutionPreset::FullHighDefinition))
                m_GameResolutionPreset = GameResolutionPreset::FullHighDefinition;
            if (ImGui::Selectable(
                        "1280 x 720",
                        m_GameResolutionPreset == GameResolutionPreset::HighDefinition))
                m_GameResolutionPreset = GameResolutionPreset::HighDefinition;
            if (ImGui::Selectable(
                        "Custom",
                        m_GameResolutionPreset == GameResolutionPreset::Custom))
                m_GameResolutionPreset = GameResolutionPreset::Custom;
            ImGui::EndCombo();
        }

        if (m_GameResolutionPreset == GameResolutionPreset::Custom)
        {
            int customResolution[2] = {
                    static_cast<int>(m_GameTargetResolution.x),
                    static_cast<int>(m_GameTargetResolution.y)};
            ImGui::SetNextItemWidth(180.0f);
            if (ImGui::InputInt2("Custom Size", customResolution))
            {
                m_GameTargetResolution.x =
                        static_cast<uint32_t>(std::max(customResolution[0], 1));
                m_GameTargetResolution.y =
                        static_cast<uint32_t>(std::max(customResolution[1], 1));
            }
        }

        ImGui::SameLine();
        ImGui::Checkbox("Show UI", &m_ShowGameUserInterface);

        const ImVec2 gameAvailableSize = ImGui::GetContentRegionAvail();
        m_GameViewportPanelSize = {
                std::max(gameAvailableSize.x, 1.0f),
                std::max(gameAvailableSize.y, 1.0f)};
        const float gameImageScale = std::min(
                m_GameViewportPanelSize.x / static_cast<float>(m_GameTargetResolution.x),
                m_GameViewportPanelSize.y / static_cast<float>(m_GameTargetResolution.y));
        const glm::vec2 gameImageSize = glm::vec2(m_GameTargetResolution) * gameImageScale;
        const glm::vec2 gameImageOffset =
                (m_GameViewportPanelSize - gameImageSize) * 0.5f;
        const ImVec2 gameContentStart = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(
                ImVec2(gameContentStart.x + gameImageOffset.x,
                       gameContentStart.y + gameImageOffset.y));
        const ImVec2 gameImageMinimum = ImGui::GetCursorScreenPos();
        ImGui::Image(
                reinterpret_cast<void*>(
                        static_cast<uint64_t>(
                                m_GameFramebuffer->GetColorAttachmentRendererID())),
                ImVec2(gameImageSize.x, gameImageSize.y),
                ImVec2(0, 1), ImVec2(1, 0));
        m_GameViewportBounds[0] = {gameImageMinimum.x, gameImageMinimum.y};
        m_GameViewportBounds[1] = {
                gameImageMinimum.x + gameImageSize.x,
                gameImageMinimum.y + gameImageSize.y};
        m_GameViewportFocused = ImGui::IsWindowFocused();
        m_GameViewportHovered = ImGui::IsWindowHovered();

        if (!m_GameViewHasValidPrimaryCamera)
        {
            const char* errorMessage = "No valid Primary Camera";
            const ImVec2 textSize = ImGui::CalcTextSize(errorMessage);
            ImGui::GetWindowDrawList()->AddText(
                    ImVec2(
                            gameImageMinimum.x + (gameImageSize.x - textSize.x) * 0.5f,
                            gameImageMinimum.y + (gameImageSize.y - textSize.y) * 0.5f),
                    ImGui::GetColorU32(ImGuiCol_Text), errorMessage);
        }
        ImGui::End();

        if (m_RequestSceneViewportFocus && TrySelectSceneViewportTab())
            m_RequestSceneViewportFocus = false;

        Application::Get().GetImGuiLayer()->BlockEvents(
                !(m_ViewportHovered
                  || (m_SceneState == SceneState::Play && m_GameViewportHovered)));
        if (IsBuildPipelineBusy())
            Application::Get().GetImGuiLayer()->BlockEvents(true);
    }

    void EditorLayer::OnEvent(Himii::Event &event)
    {
        if (IsBuildPipelineBusy())
            return;

        m_CameraController.OnEvent(event);
        if (m_ViewportHovered)
            m_EditorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }


    bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
    {
        if (e.IsRepeat())
            return false;
        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (control)
                {
                    NewScene();
                }
                break;
            }
            case Key::O:
            {
                if (control)
                {
                    OpenScene();
                }
                break;
            }
            case Key::S:
            {
                if (control && shift)
                {
                    SaveSceneAs();
                }
                else if (control)
                {
                    SaveProject();
                }
                break;
            }
            // Scene Command
            case Key::D:
            {
                if (control)
                {
                    OnDuplicateEntity();
                }
                break;
            }
            // Gizmo
            case Key::Q:
            {
                if (!ImGuizmo::IsUsing() && !ImGui::GetIO().WantTextInput)
                    m_GizmoType = -1;
                break;
            }
            case Key::W:
            {
                if (!ImGuizmo::IsUsing() && !ImGui::GetIO().WantTextInput)
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E:
            {
                if (!ImGuizmo::IsUsing() && !ImGui::GetIO().WantTextInput)
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R:
            {
                if (!ImGuizmo::IsUsing() && !ImGui::GetIO().WantTextInput)
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
            case Key::Delete:
            {
                if (m_SceneState == SceneState::Edit
                    && Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
                {
                    Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
                    if (selectedEntity)
                    {
                        m_SceneHierarchyPanel.SetSelectedEntity({});
                        m_CommandHistory.Execute(CreateScope<DeleteEntityCommand>(
                            m_EditorScene, selectedEntity,
                            [this](Entity restoredEntity)
                            {
                                m_SceneHierarchyPanel.SetSelectedEntity(restoredEntity);
                            }));
                    }
                }
                break;
            }
            case Key::Z:
            {
                if (control && m_SceneState == SceneState::Edit && !ImGui::GetIO().WantTextInput)
                {
                    if (shift)
                        m_CommandHistory.Redo();
                    else
                        m_CommandHistory.Undo();
                }
                break;
            }
            case Key::Y:
            {
                if (control && m_SceneState == SceneState::Edit && !ImGui::GetIO().WantTextInput)
                    m_CommandHistory.Redo();
                break;
            }
        }
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
    {
        if (e.GetMouseButton() == Mouse::ButtonLeft)
        {
            bool blockViewportSelectionPick = false;
            if (m_SceneState == SceneState::Edit && IsTilemapPaintActive())
            {
                Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
                if (selectedEntity && selectedEntity.HasComponent<TilemapComponent>())
                    blockViewportSelectionPick = true;
            }

            if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt)
                && m_SceneState == SceneState::Edit && !blockViewportSelectionPick)
            {
                Entity pickedEntity = m_HoveredEntity;
                if (pickedEntity && pickedEntity.HasComponent<RectTransformComponent>())
                {
                    const glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
                    glm::vec2 viewportMouse = {Input::GetMouseX() - m_ViewportBounds[0].x,
                                              Input::GetMouseY() - m_ViewportBounds[0].y};
                    viewportMouse.y = viewportSize.y - viewportMouse.y;

                    const glm::vec3 worldPosition = TilemapEditorUtility::ViewportMouseToWorldOnPlane(
                            viewportMouse, viewportSize, m_EditorCamera.GetViewProjection(), 0.0f);
                    const bool insideDesignFrame =
                            m_EditorScene->IsWorldPositionInsideDesignFrame(glm::vec2(worldPosition));

                    // Scene UI 关闭时不点选 UI；设计框外也不选 UI。
                    if (!m_ShowSceneUserInterface || !insideDesignFrame)
                        pickedEntity = {};
                }

                m_SceneHierarchyPanel.SetSelectedEntity(pickedEntity);
            }
        }
        return false;
    }

    void EditorLayer::UpdateTilemapHoverFromInput()
    {
        if (m_SceneState != SceneState::Edit || !IsTilemapEditContextActive())
            return;

        Entity selectedEntity;
        Ref<TileMapData> mapData;
        const TransformComponent *transformComponent = nullptr;
        if (!TryGetTilemapPaintContext(selectedEntity, mapData, transformComponent))
            return;

        const glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f)
            return;

        glm::vec2 viewportMouse = {Input::GetMouseX() - m_ViewportBounds[0].x,
                                   Input::GetMouseY() - m_ViewportBounds[0].y};
        viewportMouse.y = viewportSize.y - viewportMouse.y;

        const glm::vec3 worldTranslation = m_EditorScene->GetEntityWorldTranslation(selectedEntity);
        const glm::vec3 worldPosition = TilemapEditorUtility::ViewportMouseToWorldOnPlane(
                viewportMouse, viewportSize, m_EditorCamera.GetViewProjection(), worldTranslation.z);

        const glm::mat4 inverseEntityTransform =
                glm::inverse(m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity));
        const glm::vec3 localPosition = inverseEntityTransform * glm::vec4(worldPosition, 1.0f);

        m_TilemapHoveredTile =
                TilemapEditorUtility::LocalPositionToTileCoordinates(glm::vec2(localPosition), *mapData);
        m_TileMapEditorPanel.SetHoveredTileCoordinates(m_TilemapHoveredTile);
    }

    void EditorLayer::OnOverlayRender()
    {
        if (m_SceneState == SceneState::Edit)
            UpdateTilemapHoverFromInput();

        if (m_SceneState == SceneState::Play)
        {
            Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
            if (!camera)
                return;

            Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera,
                                   m_ActiveScene->GetEntityWorldTransformMatrix(camera));
        }
        else
        {
            Renderer2D::BeginScene(m_EditorCamera);
        }

        if (m_ShowGrid)
        {
            bool is2D = false;
            if (Project::GetActive())
                 is2D = Project::GetActive()->GetConfig().Is2D;

            if (m_SceneState == SceneState::Play)
            {
               // Scene::OnUpdateRuntime calls RenderScene calls DrawGrid. 
               // Do we need to draw it again? Probably not. 
               // But if we do, it should be correct.
            }
            else
            {
                const bool usePerspectiveGridPlaneRotation =
                    is2D && !m_EditorCamera.IsOrthographicProjection();
                Renderer3D::DrawGrid(m_EditorCamera, usePerspectiveGridPlaneRotation);
            }
        }

        if (m_ShowPhysicsColliders)
        {
            // Box Colliders
            {
                auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                view.each(
                        [&](auto entity, auto &tc, auto &bc2d)
                        {
                            glm::vec3 translation = tc.Position + glm::vec3(bc2d.Offset, 0.001f);
                            glm::vec3 scale = tc.Scale * glm::vec3(bc2d.Size, 1.0f);

                            glm::mat4 transform =
                                    glm::translate(glm::mat4(1.0f), translation) 
                                * glm::rotate(glm::mat4(1.0f),tc.Rotation.z,glm::vec3(0.0f,0.0f,1.0f))
                                * glm::translate(glm::mat4(1.0f),glm::vec3(bc2d.Offset,0.001f))
                                * glm::scale(glm::mat4(1.0f), scale);

                            Renderer2D::DrawRect(transform, glm::vec4(0, 1, 0, 1));
                        });
            }

            // Circle Colliders
            {
                auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
                view.each(
                        [&](auto entity, auto &tc, auto &cc2d)
                        {
                            glm::vec3 translation = tc.Position + glm::vec3(cc2d.Offset, 0.001f);
                            glm::vec3 scale = tc.Scale * glm::vec3(cc2d.Radius * 2.0f);

                            glm::mat4 transform =
                                    glm::translate(glm::mat4(1.0f), translation) * glm::scale(glm::mat4(1.0f), scale);

                            Renderer2D::DrawCircle(transform, glm::vec4(0, 1, 0, 1), 0.01f);
                        });
            }

            {
                auto assetManager = ResourceSystem::GetAssetManager();
                auto view = m_ActiveScene->GetAllEntitiesWith<TransformComponent, TilemapComponent,
                                                               TilemapCollider2DComponent>();
                view.each([&](auto entityHandle, auto& transformComponent,
                              auto& tilemapComponent, auto& tilemapColliderComponent)
                {
                    if (!tilemapColliderComponent.Enabled || tilemapComponent.TileMapHandle == 0
                        || !assetManager)
                        return;

                    Ref<TileMapData> mapData = std::static_pointer_cast<TileMapData>(
                            assetManager->GetAsset(tilemapComponent.TileMapHandle));
                    if (!mapData)
                        return;

                    Ref<TileSet> tileSet;
                    if (mapData->GetTileSetHandle() != 0)
                    {
                        tileSet = std::static_pointer_cast<TileSet>(
                                assetManager->GetAsset(mapData->GetTileSetHandle()));
                    }

                    const float cellSize = mapData->GetCellSize();
                    if (cellSize <= 0.0f)
                        return;

                    Entity sceneEntity = {entityHandle, m_ActiveScene.get()};
                    const glm::mat4 entityTransform =
                            m_ActiveScene->GetEntityWorldTransformMatrix(sceneEntity);
                    const glm::vec3 worldScale = m_ActiveScene->GetEntityWorldScale(sceneEntity);
                    const glm::vec4 colliderColor = {0.0f, 1.0f, 0.0f, 1.0f};

                    mapData->ForEachTile([&](int32_t tileX, int32_t tileY, uint16_t tileIdentifier)
                    {
                        if (!tileSet
                            || !TilemapColliderBuilder::ShouldCellCollide(tileIdentifier, *tileSet))
                            return;

                        const glm::vec2 tileCenter =
                                TileMapCoordinateUtility::TileLocalCenter(tileX, tileY, cellSize);
                        glm::mat4 colliderTransform = entityTransform
                                * glm::translate(glm::mat4(1.0f), glm::vec3(tileCenter.x, tileCenter.y, 0.001f))
                                * glm::scale(glm::mat4(1.0f),
                                             glm::vec3(cellSize * worldScale.x,
                                                       cellSize * worldScale.y, 1.0f));
                        Renderer2D::DrawRect(colliderTransform, colliderColor);
                    });
                });
            }
        }
        // Draw selected entity outline
        if (m_SceneState==SceneState::Edit)
        {
            if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity())
            {
                if (selectedEntity.HasComponent<TransformComponent>())
                {
                    glm::mat4 outlineTransform =
                            m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity);

                    if (selectedEntity.HasComponent<SpriteRendererComponent>())
                    {
                        const SpriteRendererComponent &spriteRenderer =
                                selectedEntity.GetComponent<SpriteRendererComponent>();
                        if (auto assetManager = ResourceSystem::GetAssetManager())
                        {
                            const SpriteResolved resolved = ResolveSpriteRendererDrawable(
                                selectedEntity, spriteRenderer, assetManager.get());
                            outlineTransform =
                                GetSpriteRendererVisualTransform(outlineTransform, resolved);
                        }
                    }

                    Renderer2D::DrawRect(outlineTransform, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
                }
            }

            DrawTilemapEditOverlay();
        }

        Renderer2D::EndScene();

        if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity())
        {
            if (selectedEntity.HasComponent<RectTransformComponent>()
                && m_EditorScene->IsEntityUnderCanvas(selectedEntity)
                && !selectedEntity.HasComponent<CanvasComponent>())
            {
                Renderer2D::BeginScene(m_EditorCamera);

                const glm::mat4 designToWorld = m_EditorScene->GetDesignToWorldMatrix();
                const Scene::ResolvedRectTransform resolvedRectTransform =
                        m_EditorScene->ResolveRectTransform(
                                selectedEntity,
                                static_cast<float>(m_GameTargetResolution.x),
                                static_cast<float>(m_GameTargetResolution.y));
                glm::vec2 highlightSize = resolvedRectTransform.Size;
                const glm::mat4 highlightTransform = designToWorld
                        * resolvedRectTransform.WorldTransform
                        * glm::scale(glm::mat4(1.0f), glm::vec3(highlightSize.x, highlightSize.y, 1.0f));

                Renderer2D::DrawRect(highlightTransform, glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));

                Renderer2D::EndScene();
            }
        }
    }

    void EditorLayer::CreateProject(const std::filesystem::path& projectPath, bool is2D)
    {
        if (std::filesystem::exists(projectPath))
        {
            // 如果目录已存在且非空，可能需要警告，这里简化处理只判断是否存在
            // Project::CreateProjectFiles 会处理文件生成
        }
        else
        {
            std::filesystem::create_directories(projectPath);
        }

        std::string projectName = projectPath.filename().string();
        std::filesystem::path newProjectFilePath = projectPath / (projectName + ".hproj");

        // 2. 生成物理文件 (csproj 和 assets 文件夹)
        Project::CreateProjectFiles(projectName, projectPath);

        //创建临时场景
        Ref<Scene> startScene = CreateRef<Scene>();
        startScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        {
            Entity cameraEntity = startScene->CreateEntity("Main Camera");
            auto &cc = cameraEntity.AddComponent<CameraComponent>();
            
            if (is2D)
            {
                cc.Camera.SetProjectionType(SceneCamera::ProjectionType::Orthographic);
                cameraEntity.GetComponent<TransformComponent>().Position = {0.0f, 0.0f, 10.0f};
            }
            else
            {
                cc.Camera.SetProjectionType(SceneCamera::ProjectionType::Perspective);
                cameraEntity.GetComponent<TransformComponent>().Position = {0.0f, 0.0f, 10.0f};
                // 透视相机可能需要旋转一下看清东西，或者默认朝向 -Z
            }
        }

        std::filesystem::path relativeScenePath = "scenes/Start.himii";
        std::filesystem::path fullScenePath = projectPath / "assets" / relativeScenePath;

        SceneSerializer sceneSerializer(startScene);
        sceneSerializer.Serialize(fullScenePath.string());

        HIMII_CORE_INFO("Created default scene at {0}", fullScenePath.string());

        // 2. 在内存中配置 Project 对象
        Ref<Project> newProject = Project::New();
        newProject->GetConfig().Name = projectName;
        newProject->GetConfig().AssetDirectory = "assets"; // 相对路径
        newProject->GetConfig().StartScene = relativeScenePath;
        newProject->GetConfig().ScriptModulePath = "bin/Debug/GameAssembly.dll";
        newProject->GetConfig().Is2D = is2D;

        // 3. 序列化 .hproj 文件
        Project::SaveActive(newProjectFilePath);

        // 4. 立即加载这个新项目
        OpenProject(newProjectFilePath);

        HIMII_CORE_INFO("New project created and loaded: {0}", newProjectFilePath.string());
    }

    void EditorLayer::OpenProject(const std::filesystem::path &path)
    {
        if (Project::Load(path))
        {
            auto projectDir = Project::GetProjectDirectory();
            Project::SyncScriptCoreToProjectDirectory(projectDir);
            Project::EnsureSeededDefaultAssets();
            Project::InitializeGameplayDefaultFont();
            RefreshEditorSkyboxForActiveProject();
            m_ProjectSettingsPanel.Reset();

            // 更新最近项目列表
            {
                std::string projectName = Project::GetConfig().Name;
                std::string projectPath = path.string();
                
                // remove existing if present
                auto it = std::remove_if(m_RecentProjects.begin(), m_RecentProjects.end(), [&](const RecentProject& p){
                    return p.FilePath == projectPath;
                });
                m_RecentProjects.erase(it, m_RecentProjects.end());

                m_RecentProjects.insert(m_RecentProjects.begin(), {projectName, projectPath, std::time(nullptr)});
                
                // keep max 5?
                if(m_RecentProjects.size() > 5)
                    m_RecentProjects.resize(5);

                SaveRecentProjects();
            }

            // 假设 csproj 就在项目根目录
            m_CSharpProjectPath = projectDir / "GameAssembly.csproj";

            SetupScriptFileWatchers();
            m_ScriptsDirty = false;
            m_NeedsScriptRebuild = false;

            if (IsScriptAssemblyUpToDate())
            {
                const auto assemblyPath = ScriptEngine::GetGameAssemblyDllPath();
                if (ScriptEngine::LoadAppAssembly(assemblyPath))
                {
                    HIMII_CORE_INFO("Skipped script compile: GameAssembly.dll is up to date.");
                }
                else
                {
                    HIMII_CORE_WARNING("Failed to load existing GameAssembly.dll, compiling instead.");
                    CompileAndReloadScripts();
                }
            }
            else
            {
                CompileAndReloadScripts();
            }

            // 重置 ContentBrowser 面板
            m_ContentBrowserPanel.Refresh();

            // 加载默认场景 (StartScene)
            auto startScenePath = Project::GetAssetDirectory() / Project::GetConfig().StartScene;
            if (!Project::GetConfig().StartScene.empty() && std::filesystem::exists(startScenePath))
            {
                OpenScene(startScenePath);
            }
            else
            {
                NewScene();
            }

            UpdateEditorCameraForActiveProject();
            UpdateMainWindowTitle();

            // 从 Hub / 对话框进入工程时进入编辑态，应显示 Scene ViewPort 而非 Game。
            m_RequestSceneViewportFocus = true;
            m_RequestGameViewportFocus = false;
        }
    }

    void EditorLayer::RefreshEditorSkyboxForActiveProject()
    {
        if (!Project::GetActive())
            return;

        if (Project::GetConfig().Is2D)
            return;

        static const char *skyboxFaceNames[] = {"right", "left", "top", "bottom", "front", "back"};
        std::vector<std::string> projectSkyboxFaces;
        projectSkyboxFaces.reserve(6);
        for (const char *faceName : skyboxFaceNames)
        {
            const std::filesystem::path facePath =
                    Project::GetAssetFileSystemPath(std::filesystem::path("skybox") / (std::string(faceName) + ".bmp"));
            if (!std::filesystem::exists(facePath))
            {
                HIMII_CORE_INFO("Project skybox incomplete; keeping engine editor skybox.");
                return;
            }
            projectSkyboxFaces.push_back(facePath.string());
        }

        Ref<TextureCube> projectSkybox = TextureCube::Create(projectSkyboxFaces);
        if (!projectSkybox)
        {
            HIMII_CORE_WARNING("Failed to load project skybox; keeping engine editor skybox.");
            return;
        }

        m_SkyboxTexture = projectSkybox;
        if (m_EditorScene)
            m_EditorScene->SetSkybox(m_SkyboxTexture);
        if (m_ActiveScene && m_ActiveScene != m_EditorScene)
            m_ActiveScene->SetSkybox(m_SkyboxTexture);
    }

    bool EditorLayer::TrySelectSceneViewportTab()
    {
        ImGuiWindow *viewportWindow = ImGui::FindWindowByName("ViewPort");
        if (!viewportWindow)
            return false;

        ImGuiDockNode *dockNode = viewportWindow->DockNode;
        if (!dockNode)
        {
            ImGui::FocusWindow(viewportWindow);
            return true;
        }

        // 必须在改写前读取：本帧 TabBar 可能已按 ini / 后 Begin(Game) 选中了 Game。
        const bool alreadySelected =
                (dockNode->TabBar && dockNode->TabBar->SelectedTabId == viewportWindow->TabId)
                || (!dockNode->TabBar && dockNode->SelectedTabId == viewportWindow->TabId);

        dockNode->SelectedTabId = viewportWindow->TabId;
        if (dockNode->TabBar)
            dockNode->TabBar->NextSelectedTabId = viewportWindow->TabId;

        ImGui::FocusWindow(viewportWindow);
        return alreadySelected;
    }

    void EditorLayer::UpdateEditorCameraForActiveProject()
    {
        const bool is_two_dimensional_project =
            Project::GetActive() && Project::GetActive()->GetConfig().Is2D;
        m_EditorCamera.SetOrthographicProjection(is_two_dimensional_project);
    }

    void EditorLayer::UpdateMainWindowTitle()
    {
        std::string windowTitle = "Himii Editor";
        if (Project::GetActive())
            windowTitle = Project::GetConfig().Name + " - Himii Editor";

        Application::Get().GetWindow().SetTitle(windowTitle);
    }

    void EditorLayer::DrawMainMenuBar()
    {
        if (!ImGui::BeginMenuBar())
            return;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Project"))
                SaveProject();
            if (ImGui::MenuItem("Project Settings..."))
                m_ShowProjectSettings = true;
            if (ImGui::MenuItem("Build Project...", nullptr, false, !IsBuildPipelineBusy()))
                BuildProject();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Preferences..."))
                m_ShowEditorPreferences = true;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options"))
        {
            if (ImGui::MenuItem("New Scene..", "Ctrl+N"))
                NewScene();

            if (ImGui::MenuItem("Open Scene..", "Ctrl+O"))
                OpenScene();

            if (ImGui::MenuItem("Save Scene As..", "Ctrl+Shift+S"))
                SaveSceneAs();

            if (ImGui::MenuItem("Quit"))
                Application::Get().Close();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Script"))
        {
            bool compiling = ScriptCompiler::IsCompiling();
            if (ImGui::MenuItem("Compile and Reload", nullptr, false, !compiling))
                CompileAndReloadScripts();
            if (ImGui::MenuItem("Open C# Solution"))
                OpenCSProject();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Animation Editor", nullptr, &m_ShowAnimationEditorPanel);
            ImGui::MenuItem("Texture Inspector", nullptr, &m_ShowTextureInspector);
            ImGui::MenuItem("Material Editor", nullptr, &m_ShowMaterialEditor);
            ImGui::MenuItem("TileMap Setup", nullptr, &m_ShowTileMapEditor);
            ImGui::MenuItem("Particle Emitter Editor", nullptr, &m_ShowParticleEmitterEditor);
            ImGui::MenuItem("Script Console", nullptr, &m_ShowScriptConsole);
            ImGui::MenuItem("Console", nullptr, &m_ShowConsole);
            ImGui::MenuItem("Font Diagnostics", nullptr, &m_ShowFontDiagnostics);
            ImGui::MenuItem("Show Grid", nullptr, &m_ShowGrid);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    bool EditorLayer::FlushProjectStateForBuild(std::string& errorMessage)
    {
        if (!Project::GetActive())
        {
            errorMessage = "No active project.";
            return false;
        }

        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        if (m_AnimationEditorPanel.HasUnsavedAnimationWithoutDiskPath())
        {
            errorMessage =
                    "Animation Editor has unsaved changes with no file path. "
                    "Save the .anim asset (Save As) before building.";
            return false;
        }

        if (m_TextureInspectorPanel.SaveActiveTextureMeta())
            HIMII_CORE_INFO("Build Project: texture import meta saved.");

        if (m_MaterialEditorPanel.SaveActiveMaterialAsset())
            HIMII_CORE_INFO("Build Project: material asset saved.");

        if (m_TileMapEditorPanel.SaveActiveTileMapAssets())
            HIMII_CORE_INFO("Build Project: TileMap assets saved.");

        if (m_AnimationEditorPanel.SaveActiveAnimationAsset())
            HIMII_CORE_INFO("Build Project: animation asset saved.");

        const ProjectConfig& projectConfiguration = Project::GetConfig();
        if (projectConfiguration.StartScene.empty())
        {
            errorMessage = "Project Start Scene is empty. Set Project Settings → Start Scene before building.";
            return false;
        }

        const std::filesystem::path startSceneAbsolutePath =
                Project::GetAssetDirectory() / projectConfiguration.StartScene;
        if (!std::filesystem::exists(startSceneAbsolutePath))
        {
            errorMessage = "Start Scene file missing: " + startSceneAbsolutePath.string();
            return false;
        }

        std::error_code startSceneCanonicalError;
        const std::filesystem::path startSceneCanonical =
                std::filesystem::weakly_canonical(startSceneAbsolutePath, startSceneCanonicalError);

        std::error_code editorSceneCanonicalError;
        const std::filesystem::path editorSceneCanonical =
                m_EditorScenePath.empty()
                        ? std::filesystem::path{}
                        : std::filesystem::weakly_canonical(m_EditorScenePath, editorSceneCanonicalError);

        const bool editorSceneIsStartScene =
                !startSceneCanonicalError
                && !editorSceneCanonicalError
                && !editorSceneCanonical.empty()
                && !startSceneCanonical.empty()
                && editorSceneCanonical == startSceneCanonical;

        if (!m_EditorScenePath.empty() && m_EditorScene)
        {
            SerializeScene(m_EditorScene, m_EditorScenePath);
            HIMII_CORE_INFO("Build Project: scene saved to {0}", m_EditorScenePath.string());
        }

        if (!editorSceneIsStartScene)
        {
            HIMII_CORE_WARNING(
                    "Build Project: open scene is not the Start Scene. "
                    "Package will launch '{0}' from disk (current editor scene is not the start scene).",
                    projectConfiguration.StartScene.string());
        }

        if (m_EditorScenePath.empty())
        {
            HIMII_CORE_WARNING(
                    "Build Project: current editor scene has no saved path. "
                    "Only the Start Scene on disk will be included as the launch scene.");
        }

        Ref<AssetManager> assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
        {
            errorMessage = "AssetManager is unavailable.";
            return false;
        }

        assetManager->SerializeAssetRegistry();

        const std::filesystem::path assetRegistryPath = Project::GetAssetRegistryPath();
        if (!std::filesystem::exists(assetRegistryPath))
        {
            errorMessage = "AssetRegistry.yaml missing after save: " + assetRegistryPath.string();
            return false;
        }

        return true;
    }

    void EditorLayer::BuildProject()
    {
        if (IsBuildPipelineBusy())
            return;

        if (!Project::GetActive())
        {
            HIMII_CORE_ERROR("Build Project: no active project.");
            return;
        }

        std::string flushErrorMessage;
        if (!FlushProjectStateForBuild(flushErrorMessage))
        {
            HIMII_CORE_ERROR("Build Project failed: {0}", flushErrorMessage);
            m_BuildPipelineUiState = BuildPipelineUiState::Failed;
            m_BuildResultMessage = flushErrorMessage;
            m_BuildProgressStageLabel = "Build failed";
            m_BuildProgressNormalized = 0.0f;
            m_BuildProgressModalOpenRequested = true;
            return;
        }

        const std::string filepath = FileDialog::SaveFile("Executable (*.exe)\0*.exe\0");
        if (filepath.empty())
            return;

        const ProjectBuildPipelineResult beginResult =
                ProjectBuildPipeline::BeginSession(std::filesystem::path(filepath));
        if (!beginResult.Succeeded)
        {
            HIMII_CORE_ERROR("Build Project failed: {0}", beginResult.ErrorMessage);
            m_BuildPipelineUiState = BuildPipelineUiState::Failed;
            m_BuildResultMessage = beginResult.ErrorMessage;
            m_BuildProgressStageLabel = "Build failed";
            m_BuildProgressNormalized = 0.0f;
            m_BuildProgressModalOpenRequested = true;
            return;
        }

        m_BuildOutputDirectory = std::filesystem::path(filepath).parent_path();
        m_BuildPipelineUiState = BuildPipelineUiState::Running;
        m_BuildProgressStageLabel = "Compiling Release GameAssembly...";
        m_BuildProgressNormalized = 0.1f;
        m_BuildResultMessage.clear();
        m_BuildProgressModalOpenRequested = true;
    }

    bool EditorLayer::IsBuildPipelineBusy() const
    {
        return m_BuildPipelineUiState == BuildPipelineUiState::Running
                || ProjectBuildPipeline::IsSessionActive();
    }

    void EditorLayer::UpdateBuildPipelineSession()
    {
        if (m_BuildPipelineUiState != BuildPipelineUiState::Running)
            return;

        ProjectBuildPipelineProgress progress;
        const bool stillRunning = ProjectBuildPipeline::UpdateSession(progress);
        m_BuildProgressStageLabel = progress.StageLabel;
        m_BuildProgressNormalized = progress.ProgressNormalized;

        if (stillRunning)
            return;

        const ProjectBuildPipelineResult sessionResult = ProjectBuildPipeline::GetSessionResult();
        m_BuildOutputDirectory = ProjectBuildPipeline::GetSessionOutputDirectory();

        if (sessionResult.Succeeded)
        {
            m_BuildPipelineUiState = BuildPipelineUiState::Succeeded;
            m_BuildResultMessage = "Build completed successfully.";
            m_BuildProgressNormalized = 1.0f;
            m_BuildProgressStageLabel = "Build succeeded";
            HIMII_CORE_INFO("Build Project completed: {0}", m_BuildOutputDirectory.string());
            OpenBuildOutputDirectory(m_BuildOutputDirectory);
        }
        else
        {
            m_BuildPipelineUiState = BuildPipelineUiState::Failed;
            m_BuildResultMessage = sessionResult.ErrorMessage.empty()
                                           ? "Build failed."
                                           : sessionResult.ErrorMessage;
            HIMII_CORE_ERROR("Build Project failed: {0}", m_BuildResultMessage);
        }
    }

    void EditorLayer::DrawBuildProgressModal()
    {
        if (m_BuildProgressModalOpenRequested)
        {
            ImGui::OpenPopup("Build Project");
            m_BuildProgressModalOpenRequested = false;
        }

        // Escape can dismiss ImGui modals; keep the progress dialog open while packaging runs (C1).
        if (m_BuildPipelineUiState == BuildPipelineUiState::Running && !ImGui::IsPopupOpen("Build Project"))
            ImGui::OpenPopup("Build Project");

        if (m_BuildPipelineUiState == BuildPipelineUiState::Idle)
            return;

        ImGui::SetNextWindowSize(ImVec2(420.0f, 0.0f), ImGuiCond_Appearing);
        if (!ImGui::BeginPopupModal("Build Project", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
            return;

        ImGui::TextUnformatted(m_BuildProgressStageLabel.c_str());
        ImGui::ProgressBar(m_BuildProgressNormalized, ImVec2(-1.0f, 0.0f));

        if (m_BuildPipelineUiState == BuildPipelineUiState::Running)
        {
            ImGui::TextUnformatted("Please wait. The editor is locked until the build finishes.");
        }
        else if (m_BuildPipelineUiState == BuildPipelineUiState::Succeeded)
        {
            ImGui::TextWrapped("%s", m_BuildResultMessage.c_str());
            if (!m_BuildOutputDirectory.empty())
                ImGui::TextWrapped("Output: %s", m_BuildOutputDirectory.string().c_str());
            if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
            {
                m_BuildPipelineUiState = BuildPipelineUiState::Idle;
                ImGui::CloseCurrentPopup();
            }
        }
        else if (m_BuildPipelineUiState == BuildPipelineUiState::Failed)
        {
            ImGui::TextWrapped("%s", m_BuildResultMessage.c_str());
            if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
            {
                m_BuildPipelineUiState = BuildPipelineUiState::Idle;
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }

    void EditorLayer::OpenBuildOutputDirectory(const std::filesystem::path& directoryPath)
    {
        if (directoryPath.empty() || !std::filesystem::exists(directoryPath))
            return;

#ifdef HIMII_PLATFORM_WINDOWS
        const std::wstring wideDirectoryPath = directoryPath.wstring();
        const HINSTANCE shellResult = ShellExecuteW(
                nullptr, L"open", wideDirectoryPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<INT_PTR>(shellResult) <= 32)
        {
            HIMII_CORE_WARNING("Failed to open build output directory: {0}", directoryPath.string());
        }
#else
        HIMII_CORE_INFO("Build output directory: {0}", directoryPath.string());
#endif
    }

    void EditorLayer::OpenCSProject()
    {
        if (!Project::GetActive())
            return;

        auto projectDir = Project::GetProjectDirectory();
        std::string projectName = Project::GetConfig().Name;

        if (!ScriptIDELauncher::OpenProject(projectDir, projectName))
            HIMII_CORE_WARNING("Failed to open script IDE for project: {0}", projectName);
    }

    void EditorLayer::NewScene()
    {
        m_SceneHierarchyPanel.SetSelectedEntity({});
        m_HoveredEntity = {};

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_World->SetActiveScene(m_ActiveScene);
        m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        if (m_SkyboxTexture)
            m_ActiveScene->SetSkybox(m_SkyboxTexture);

        const bool isTwoDimensional =
                Project::GetActive() && Project::GetActive()->GetConfig().Is2D;
        if (!isTwoDimensional)
        {
            Entity lightEntity = m_ActiveScene->CreateEntity("Directional Light");
            auto &lightTransform = lightEntity.GetComponent<TransformComponent>();
            lightTransform.Rotation = glm::radians(glm::vec3(50.0f, -30.0f, 0.0f));
            auto &lightComponent = lightEntity.AddComponent<LightComponent>();
            lightComponent.Type = LightType::Directional;
            lightComponent.Color = glm::vec4(1.0f);
            lightComponent.Intensity = 1.0f;
            lightComponent.Enabled = true;

            Entity environmentEntity = m_ActiveScene->CreateEntity("Environment");
            auto &environmentComponent = environmentEntity.AddComponent<EnvironmentComponent>();
            environmentComponent.AmbientColor = glm::vec4(1.0f);
            environmentComponent.AmbientIntensity = 0.15f;
            environmentComponent.Enabled = true;
        }

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        m_EditorScenePath = std::filesystem::path();
    }

    void EditorLayer::LoadRecentProjects()
    {
        m_RecentProjects.clear();
        std::filesystem::path recentProjectsPath = "recent_projects.yaml";
        if (std::filesystem::exists(recentProjectsPath))
        {
            try
            {
                YAML::Node data = YAML::LoadFile(recentProjectsPath.string());
                auto projects = data["RecentProjects"];
                if (projects)
                {
                    for (auto project : projects)
                    {
                        RecentProject p;
                        p.Name = project["Name"].as<std::string>();
                        p.FilePath = project["FilePath"].as<std::string>();
                        p.LastOpened = project["LastOpened"].as<long long>();

                        // Filter out non-existing projects
                        if(std::filesystem::exists(p.FilePath))
                            m_RecentProjects.push_back(p);
                    }
                }
            }
            catch (YAML::ParserException &e)
            {
                HIMII_CORE_ERROR("Failed to load recent projects: {0}", e.what());
            }
        }
        // sort by last opened descending
         std::sort(m_RecentProjects.begin(), m_RecentProjects.end(), [](const RecentProject& a, const RecentProject& b) {
            return a.LastOpened > b.LastOpened;
        });
    }

    void EditorLayer::SaveRecentProjects()
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "RecentProjects" << YAML::Value << YAML::BeginSeq;
        for (const auto &project : m_RecentProjects)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Name" << YAML::Value << project.Name;
            out << YAML::Key << "FilePath" << YAML::Value << project.FilePath;
            out << YAML::Key << "LastOpened" << YAML::Value << project.LastOpened;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream fout("recent_projects.yaml");
        fout << out.c_str();
    }

    void EditorLayer::DrawStartupSplash()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoInputs |
                                        ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("Startup Splash", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        const ImVec2 window_position = ImGui::GetWindowPos();
        const ImVec2 window_size = ImGui::GetWindowSize();
        const ImVec2 window_max = {window_position.x + window_size.x, window_position.y + window_size.y};

        if (m_EngineSplashTexture && m_EngineSplashTexture->IsLoaded() &&
            m_EngineSplashTexture->GetRendererID() != 0)
        {
            const auto &texture_uv_corners = SpriteSheetUtility::FullTextureImGuiUvCorners;
            draw_list->AddImage((ImTextureID)(intptr_t)m_EngineSplashTexture->GetRendererID(), window_position,
                                window_max,
                                ImVec2(texture_uv_corners.TopLeft.x, texture_uv_corners.TopLeft.y),
                                ImVec2(texture_uv_corners.BottomRight.x, texture_uv_corners.BottomRight.y));

            const float overlay_height = (float)SplashStatusOverlayHeightPixels;
            const ImVec2 overlay_min = {window_position.x, window_max.y - overlay_height};
            const ImVec2 overlay_max = window_max;
            draw_list->AddRectFilled(overlay_min, overlay_max, IM_COL32(0, 0, 0, 160));

            const float horizontal_padding = 16.0f;
            const ImVec2 status_position = {overlay_min.x + horizontal_padding, overlay_min.y + 10.0f};
            draw_list->AddText(status_position, IM_COL32(255, 255, 255, 255),
                               m_StartupStatusMessage.c_str());

            const float progress_bar_height = 6.0f;
            const float progress_bar_bottom_padding = 12.0f;
            const ImVec2 progress_bar_min = {overlay_min.x + horizontal_padding,
                                             overlay_max.y - progress_bar_bottom_padding - progress_bar_height};
            const ImVec2 progress_bar_max = {overlay_max.x - horizontal_padding,
                                             overlay_max.y - progress_bar_bottom_padding};
            draw_list->AddRectFilled(progress_bar_min, progress_bar_max, IM_COL32(48, 48, 48, 255));

            const float progress_clamped = std::clamp(m_StartupProgress, 0.0f, 1.0f);
            const ImVec2 progress_fill_max = {
                progress_bar_min.x + (progress_bar_max.x - progress_bar_min.x) * progress_clamped,
                progress_bar_max.y};
            draw_list->AddRectFilled(progress_bar_min, progress_fill_max, IM_COL32(230, 170, 50, 255));
        }
        else
        {
            draw_list->AddRectFilled(window_position, window_max, IM_COL32(20, 20, 20, 255));
            const char *fallback_title = "Himii Engine";
            const ImVec2 title_size = ImGui::CalcTextSize(fallback_title);
            const ImVec2 title_position = {window_position.x + (window_size.x - title_size.x) * 0.5f,
                                           window_position.y + (window_size.y - title_size.y) * 0.5f};
            draw_list->AddText(title_position, IM_COL32(255, 255, 255, 255), fallback_title);
        }

        ImGui::End();
    }

    void EditorLayer::DrawProjectHub()
    {
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));

        ImGui::Begin("Project Hub", nullptr, window_flags);
        ImGui::PopStyleVar(3);
        
        ImGui::Text("Himii Engine - Project Hub");
        ImGui::Separator();
        
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 300.0f);

        // Left Column: Actions
        if (ImGui::Button("Create New Project", ImVec2(280, 50)))
        {
            ImGui::OpenPopup("New Project Setup");
        }

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("New Project Setup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char projectName[256] = "NewProject";
            static char projectPath[512] = ""; 
            if(strlen(projectPath) == 0)
                 strcpy(projectPath, std::filesystem::current_path().string().c_str());

            static int projectType = 0; // 0: 2D, 1: 3D
            
            ImGui::Text("Project Name:");
            ImGui::InputText("##project_name", projectName, 256);
            
            ImGui::Text("Location:");
            ImGui::PushItemWidth(300);
            ImGui::InputText("##project_path", projectPath, 512, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("...", ImVec2(30, 0)))
            {
                std::string folder = FileDialog::OpenFolder(projectPath);
                if (!folder.empty())
                {
                    strcpy(projectPath, folder.c_str());
                }
            }

            ImGui::Separator();
            ImGui::Text("Select Project Template:");
            ImGui::RadioButton("2D Project (Orthographic Camera)", &projectType, 0);
            ImGui::RadioButton("3D Project (Perspective Camera)", &projectType, 1);

            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            { 
               std::filesystem::path fullPath = std::filesystem::path(projectPath) / projectName;
               CreateProject(fullPath, projectType == 0);
               ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::Dummy(ImVec2(0, 10));
        if (ImGui::Button("Open Project", ImVec2(280, 50)))
        {
             std::string filepath = FileDialog::OpenFile("Himii Project (*.hproj)\0*.hproj\0");
             if (!filepath.empty())
                 OpenProject(filepath);
        }

        ImGui::NextColumn();

        // Right Column: Recent Projects
        ImGui::Text("Recent Projects");
        ImGui::Separator();
        
        ImGui::BeginChild("RecentProjectsList");
        for (const auto& project : m_RecentProjects)
        {
            std::string label = project.Name + " (" + project.FilePath + ")";
            if (ImGui::Button(label.c_str(), ImVec2(-1, 40)))
            {
                OpenProject(project.FilePath);
            }
        }
        ImGui::EndChild();

        ImGui::Columns(1);
        ImGui::End();
    }

    void EditorLayer::OpenScene()
    {
        std::string filePath = FileDialog::OpenFile("Himii Scene(*.himii)\0*.himii\0");

        if (!filePath.empty())
        {
            OpenScene(filePath);
        }
    }

    void EditorLayer::OpenScene(const std::filesystem::path &path)
    {
        if (m_SceneState != SceneState::Edit)
        {
            OnSceneStop();
        }

        m_SceneHierarchyPanel.SetSelectedEntity({});
        m_HoveredEntity = {};

        Ref<Scene> newScene = CreateRef<Scene>();
        SceneSerializer serializer(newScene);
        if (serializer.Deserialize(path.string()))
        {
            m_EditorScene = newScene;

            if (m_SkyboxTexture)
                m_EditorScene->SetSkybox(m_SkyboxTexture);
            m_SceneHierarchyPanel.SetContext(m_EditorScene);
            m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            m_ActiveScene = m_EditorScene;
            m_World->SetActiveScene(m_ActiveScene);
            m_EditorScenePath = path;

            if (Project::GetActive())
            {
                std::error_code errorCode;
                std::filesystem::path relativeScenePath =
                        std::filesystem::relative(path, Project::GetAssetDirectory(), errorCode);
                if (!errorCode)
                    ScriptEngine::SetActiveSceneRelativePath(relativeScenePath.generic_string());
            }
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!m_EditorScenePath.empty())
        {
            SerializeScene(m_ActiveScene, m_EditorScenePath);
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveProject()
    {
        // Save Project Settings (including Asset Registry)
        auto project = Project::GetActive();
        if(!project) return;
        
        auto projectDir = Project::GetProjectDirectory();
        auto appName = project->GetConfig().Name;
        std::filesystem::path projectFile = projectDir / (appName + ".hproj");
        
        if (Project::SaveActive(projectFile))
        {
            HIMII_CORE_INFO("Project saved successfully to {0}", projectFile.string());
        }
        else
        {
            HIMII_CORE_ERROR("Failed to save project to {0}", projectFile.string());
        }

        if (m_TextureInspectorPanel.SaveActiveTextureMeta())
        {
            HIMII_CORE_INFO("Texture import meta saved.");
        }

        if (m_MaterialEditorPanel.SaveActiveMaterialAsset())
        {
            HIMII_CORE_INFO("Material asset saved.");
        }

        if (m_TileMapEditorPanel.SaveActiveTileMapAssets())
        {
            HIMII_CORE_INFO("TileMap assets saved.");
        }

        if (m_AnimationEditorPanel.SaveActiveAnimationAsset())
        {
            HIMII_CORE_INFO("Animation asset saved.");
        }

        // Save Scene
        SaveScene();
    }
    
    void EditorLayer::SaveSceneAs()
    {
        std::string filePath = FileDialog::SaveFile("Himii Scene(*.himii)\0*.himii\0");
        if (!filePath.empty())
        {
            SerializeScene(m_ActiveScene, filePath);

            m_EditorScenePath = filePath;
        }
    }

    void EditorLayer::SerializeScene(Ref<Scene> scene, const std::filesystem::path &path)
    {
        SceneSerializer serializer(scene);
        serializer.Serialize(path.string());
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_ScriptsDirty || ScriptCompiler::IsCompiling() || m_NeedsScriptRebuild)
        {
            m_PendingPlayAfterCompile = true;
            // 已在编译中则只等待本轮结束，不要额外排队一轮
            if (!ScriptCompiler::IsCompiling())
                RequestScriptCompile();
            return;
        }

        StartScenePlay();
    }

    void EditorLayer::StartScenePlay()
    {
        if (m_SceneState == SceneState::Simulate)
            OnSceneStop();

        m_CommandHistory.Clear();
        ConsoleLog::Clear();

        m_RestoreSceneViewportAfterPlay =
                m_ViewportFocused || !m_GameViewportFocused;
        m_RequestGameViewportFocus = true;
        m_SceneState = SceneState::Play;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_World->SetActiveScene(m_ActiveScene);
        m_World->OnRuntimeStart();

        if (Project::GetActive() && !m_EditorScenePath.empty())
        {
            std::error_code errorCode;
            std::filesystem::path relativeScenePath =
                    std::filesystem::relative(m_EditorScenePath, Project::GetAssetDirectory(), errorCode);
            if (!errorCode)
                ScriptEngine::SetActiveSceneRelativePath(relativeScenePath.generic_string());
        }

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneSimulate()
    {
        if (m_SceneState == SceneState::Play)
            OnSceneStop();

        m_CommandHistory.Clear();
        m_SceneState = SceneState::Simulate;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_World->SetActiveScene(m_ActiveScene);
        m_World->OnSimulationStart();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneStop()
    {
        m_SceneHierarchyPanel.SetSelectedEntity({});
        const bool wasPlaying = m_SceneState == SceneState::Play;

        if (wasPlaying)
            m_World->OnRuntimeStop();
        else if (m_SceneState == SceneState::Simulate)
            m_World->OnSimulationStop();

        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;
        m_World->SetActiveScene(m_EditorScene);

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        if (wasPlaying)
        {
            m_RequestSceneViewportFocus = m_RestoreSceneViewportAfterPlay;
            m_RequestGameViewportFocus = !m_RestoreSceneViewportAfterPlay;
        }
    }

    void EditorLayer::OnDuplicateEntity()
    {
        if (m_SceneState != SceneState::Edit)
            return;

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (selectedEntity)
        {
            Entity newEntity = m_EditorScene->DuplicateEntity(selectedEntity);
            m_SceneHierarchyPanel.SetSelectedEntity(newEntity);
        }
    }

    void EditorLayer::SetupScriptFileWatchers()
    {
        auto scriptsDirectory = Project::GetAssetDirectory() / "scripts";
        m_ScriptFileWatcher.Watch(scriptsDirectory, [this]() { MarkScriptsDirtyFromWatcher(); });

        if (!m_CSharpProjectPath.empty() && std::filesystem::exists(m_CSharpProjectPath))
        {
            m_ScriptProjectFileWatcher.WatchFile(m_CSharpProjectPath, [this]() { MarkScriptsDirtyFromWatcher(); });
        }
        else
        {
            m_ScriptProjectFileWatcher.Clear();
        }
    }

    void EditorLayer::MarkScriptsDirtyFromWatcher()
    {
        m_ScriptsDirty = true;
        RequestScriptCompile();
    }

    bool EditorLayer::IsScriptAssemblyUpToDate() const
    {
        if (!Project::GetActive() || m_CSharpProjectPath.empty())
            return false;

        const auto assemblyPath = ScriptEngine::GetGameAssemblyDllPath();
        if (assemblyPath.empty() || !std::filesystem::exists(assemblyPath))
            return false;

        std::error_code errorCode;
        const auto assemblyWriteTime = std::filesystem::last_write_time(assemblyPath, errorCode);
        if (errorCode)
            return false;

        auto isNewerThanAssembly = [&](const std::filesystem::path& path) -> bool
        {
            if (!std::filesystem::exists(path))
                return false;
            std::error_code localErrorCode;
            const auto writeTime = std::filesystem::last_write_time(path, localErrorCode);
            if (localErrorCode)
                return true;
            return writeTime > assemblyWriteTime;
        };

        if (isNewerThanAssembly(m_CSharpProjectPath))
            return false;

        const auto scriptsDirectory = Project::GetAssetDirectory() / "scripts";
        if (!std::filesystem::exists(scriptsDirectory))
            return true;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsDirectory))
        {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().extension() != ".cs")
                continue;
            if (isNewerThanAssembly(entry.path()))
                return false;
        }

        return true;
    }

    void EditorLayer::RequestScriptCompile()
    {
        if (!Project::GetActive() || m_CSharpProjectPath.empty())
            return;

        if (IsBuildPipelineBusy())
        {
            m_ScriptsDirty = true;
            return;
        }

        // Play / Simulate 中只标记 dirty，不强制 Stop、不自动编译
        if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate)
        {
            m_ScriptsDirty = true;
            return;
        }

        if (ScriptCompiler::IsCompiling())
        {
            m_NeedsScriptRebuild = true;
            m_ScriptsDirty = true;
            return;
        }

        m_ScriptsDirty = true;
        ScriptEngine::RequestCompileAndReload(m_CSharpProjectPath);
    }

    void EditorLayer::CompileAndReloadScripts()
    {
        const bool wasPlaying = (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);
        if (wasPlaying)
            OnSceneStop();

        m_NotifyReloadAfterCompile = wasPlaying;
        m_NeedsScriptRebuild = false;
        m_ScriptsDirty = true;

        if (Project::GetActive())
            Project::SyncScriptCoreToProjectDirectory(Project::GetProjectDirectory());

        if (!ScriptEngine::RequestCompileAndReload(m_CSharpProjectPath))
        {
            // 已在编译中：排队，结束后再编一轮
            m_NeedsScriptRebuild = true;
            m_NotifyReloadAfterCompile = false;
        }
    }

    void EditorLayer::UI_Toolbar()
    {
        ImGuiWindowClass toolbar_window_class;
        toolbar_window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_HiddenTabBar;
        ImGui::SetNextWindowClass(&toolbar_window_class);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        auto &colors = ImGui::GetStyle().Colors;
        const auto &buttonHovered = colors[ImGuiCol_ButtonHovered];
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        const auto &buttonActive = colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

        ImGui::Begin("##toolbar", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        bool toolbarEnable = (bool)m_ActiveScene;

        ImVec4 tintColor = ImVec4(1, 1, 1, 1);

        float size = ImGui::GetWindowHeight() - 4.0f;
        
        
        {
            // Gizmo Controls (Left Aligned)
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));

            auto drawVectorIcon = [&](int type, ImVec2 pMin, ImVec2 pMax, ImU32 color) {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                float w = pMax.x - pMin.x;
                float h = pMax.y - pMin.y;
                float pad = w * 0.2f;
                ImVec2 center = { pMin.x + w * 0.5f, pMin.y + h * 0.5f };

                if (type == -1) // Select (Cursor)
                {
                    drawList->AddTriangleFilled(
                        { pMin.x + pad, pMin.y + pad },
                        { pMin.x + pad, pMax.y - pad },
                        { pMax.x - pad, pMax.y - pad * 1.5f },
                        color
                    );
                }
                else if (type == ImGuizmo::OPERATION::TRANSLATE) // Move (Arrows)
                {
                    // Vertical arrow
                    drawList->AddLine({ center.x, pMin.y + pad }, { center.x, pMax.y - pad }, color, 2.0f);
                    drawList->AddLine({ pMin.x + pad, center.y }, { pMax.x - pad, center.y }, color, 2.0f);
                    // Arrow heads
                    drawList->AddTriangleFilled({center.x, pMin.y + pad}, {center.x - 3, pMin.y + pad + 4}, {center.x + 3, pMin.y + pad + 4}, color);
                    drawList->AddTriangleFilled({pMax.x - pad, center.y}, {pMax.x - pad - 4, center.y - 3}, {pMax.x - pad - 4, center.y + 3}, color);
                }
                else if (type == ImGuizmo::OPERATION::ROTATE) // Rotate (Circle)
                {
                    drawList->AddCircle(center, w * 0.3f, color, 0, 2.0f);
                    // Add a small arrow head on the circle?
                    drawList->AddTriangleFilled({center.x, pMin.y + pad + 1}, {center.x - 3, pMin.y + pad + 5}, {center.x + 3, pMin.y + pad + 5}, color);
                }
                else if (type == ImGuizmo::OPERATION::SCALE) // Scale (Box)
                {
                    drawList->AddRect({ pMin.x + pad, pMin.y + pad }, { pMax.x - pad, pMax.y - pad }, color, 0.0f, 0, 2.0f);
                    drawList->AddRectFilled({ center.x - 2, center.y - 2 }, { center.x + 2, center.y + 2 }, color);
                }
            };
            
            auto drawGizmoButton = [&](int type) {
                bool active = m_GizmoType == type;
                if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Active Highlight
                
                ImGui::PushID(type);
                if (ImGui::Button("", ImVec2(size, size)))
                {
                    m_GizmoType = type;
                }
                // Custom Draw
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                ImU32 iconColor = active ? 0xFFFFFFFF : 0xFFAAAAAA;
                drawVectorIcon(type, pMin, pMax, iconColor);

                ImGui::PopID();
                if (active) ImGui::PopStyleColor();
                ImGui::SameLine();
            };

            drawGizmoButton(-1);
            drawGizmoButton(ImGuizmo::OPERATION::TRANSLATE);
            drawGizmoButton(ImGuizmo::OPERATION::ROTATE);
            drawGizmoButton(ImGuizmo::OPERATION::SCALE);

            {
                const bool active = m_ShowSceneUserInterface;
                if (active)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.65f, 1.0f));
                if (ImGui::Button(active ? "Scene UI: On" : "Scene UI: Off", ImVec2(100.0f, size)))
                    m_ShowSceneUserInterface = !m_ShowSceneUserInterface;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip(
                            "Show or hide UI content in the Scene view. When off, UI does not occlude or pick world objects; design frame and selection gizmos remain.");
                if (active)
                    ImGui::PopStyleColor();
                ImGui::SameLine();
            }

            ImGui::PopStyleVar(2);
        }

        {
            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit||m_SceneState==SceneState::Simulate) ? m_IconPlay : m_IconStop;
            ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
            const auto& playIconUv = SpriteSheetUtility::FullTextureImGuiUvCorners;
            if (ImGui::ImageButton("state", (ImTextureID)icon->GetRendererID(), ImVec2(size, size),
                                   ImVec2(playIconUv.TopLeft.x, playIconUv.TopLeft.y),
                                   ImVec2(playIconUv.BottomRight.x, playIconUv.BottomRight.y),
                                   ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor)
                && toolbarEnable)
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulate)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            }
        }
        ImGui::SameLine();
        {
            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit||m_SceneState==SceneState::Play) ? m_IconSimulate : m_IconStop;

            const auto& simulateIconUv = SpriteSheetUtility::FullTextureImGuiUvCorners;
            if (ImGui::ImageButton("state1", (ImTextureID)icon->GetRendererID(), ImVec2(size, size),
                                   ImVec2(simulateIconUv.TopLeft.x, simulateIconUv.TopLeft.y),
                                   ImVec2(simulateIconUv.BottomRight.x, simulateIconUv.BottomRight.y),
                                   ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tintColor)
                && toolbarEnable)
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Play)
                    OnSceneSimulate();
                else if (m_SceneState == SceneState::Simulate)
                    OnSceneStop();
            }
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    bool EditorLayer::IsTilemapMoveEntityActive()
    {
        if (m_SceneState != SceneState::Edit)
            return false;

        return m_TileMapEditorPanel.IsMoveEntityModeEnabled();
    }

    void EditorLayer::UpdateTilemapPaintSession()
    {
        if (m_SceneState != SceneState::Edit)
            return;

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        UUID currentTilemapEntityUUID = 0;
        bool isTilemapEditTarget = false;

        if (selectedEntity && selectedEntity.HasComponent<TilemapComponent>())
        {
            const TilemapComponent& tilemapComponent = selectedEntity.GetComponent<TilemapComponent>();
            if (tilemapComponent.TileMapHandle != 0 && Project::GetActive())
            {
                auto assetManager = ResourceSystem::GetAssetManager();
                if (assetManager && assetManager->IsAssetHandleValid(tilemapComponent.TileMapHandle)
                    && assetManager->GetAsset(tilemapComponent.TileMapHandle))
                {
                    isTilemapEditTarget = true;
                    currentTilemapEntityUUID = selectedEntity.GetUUID();
                }
            }
        }

        const bool tileMapEditorWindowClosed = m_TileMapEditorWasVisible && !m_ShowTileMapEditor;
        const bool leftTilemapEditTarget = m_TilemapPaintSessionEntityUUID != 0 && !isTilemapEditTarget;
        const bool switchedTilemapEntity = isTilemapEditTarget && m_TilemapPaintSessionEntityUUID != 0
            && currentTilemapEntityUUID != m_TilemapPaintSessionEntityUUID;

        if (tileMapEditorWindowClosed || leftTilemapEditTarget || switchedTilemapEntity)
        {
            m_TileMapEditorPanel.ClearScenePaintSession();
            ResetTilemapBoxSelection();
            m_TilemapHoveredTile = {TileMapCoordinateUtility::InvalidTileCoordinate,
                                    TileMapCoordinateUtility::InvalidTileCoordinate};
            if (leftTilemapEditTarget || tileMapEditorWindowClosed)
                m_TilemapPaintSessionEntityUUID = 0;
        }

        if (isTilemapEditTarget)
            m_TilemapPaintSessionEntityUUID = currentTilemapEntityUUID;

        m_TileMapEditorWasVisible = m_ShowTileMapEditor;
    }

    bool EditorLayer::IsTilemapEditContextActive()
    {
        if (m_SceneState != SceneState::Edit)
            return false;

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (!selectedEntity || !selectedEntity.HasComponent<TilemapComponent>())
            return false;

        const TilemapComponent &tilemapComponent = selectedEntity.GetComponent<TilemapComponent>();
        if (tilemapComponent.TileMapHandle == 0)
            return false;

        if (!Project::GetActive())
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(tilemapComponent.TileMapHandle))
            return false;

        return assetManager->GetAsset(tilemapComponent.TileMapHandle) != nullptr;
    }

    bool EditorLayer::IsTilemapPaintActive()
    {
        if (!IsTilemapEditContextActive() || IsTilemapMoveEntityActive())
            return false;

        return m_TileMapEditorPanel.CanPaintInScene();
    }

    bool EditorLayer::TryGetTilemapPaintContext(Entity &outEntity, Ref<TileMapData> &outMapData,
                                                TransformComponent const *&outTransform)
    {
        outEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (!outEntity || !outEntity.HasComponent<TilemapComponent>()
            || !outEntity.HasComponent<TransformComponent>())
            return false;

        const TilemapComponent &tilemapComponent = outEntity.GetComponent<TilemapComponent>();
        if (tilemapComponent.TileMapHandle == 0)
            return false;

        if (m_TileMapEditorPanel.GetTileMapHandle() != tilemapComponent.TileMapHandle)
            m_TileMapEditorPanel.Open(tilemapComponent.TileMapHandle);

        outMapData = m_TileMapEditorPanel.GetMapData();
        if (!outMapData)
            return false;

        outTransform = &outEntity.GetComponent<TransformComponent>();
        return true;
    }

    void EditorLayer::HandleTilemapScenePaint(bool allowPainting)
    {
        if (m_SceneState != SceneState::Edit)
            return;

        if (!m_ViewportHovered && !m_TilemapViewportCapture)
            return;

        Entity selectedEntity;
        Ref<TileMapData> mapData;
        const TransformComponent *transformComponent = nullptr;
        if (!TryGetTilemapPaintContext(selectedEntity, mapData, transformComponent))
            return;

        if (allowPainting && !IsTilemapPaintActive())
            return;

        if (m_TilemapViewportCapture || m_ViewportFocused || m_ViewportHovered)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_B))
            {
                m_TileMapEditorPanel.SetCurrentTool(TileMapEditorPanel::Tool::Brush);
                ResetTilemapBoxSelection();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_E))
            {
                const bool shiftHeld = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
                m_TileMapEditorPanel.SetCurrentTool(
                        shiftHeld ? TileMapEditorPanel::Tool::BoxErase : TileMapEditorPanel::Tool::Eraser);
                ResetTilemapBoxSelection();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_G))
            {
                m_TileMapEditorPanel.SetCurrentTool(TileMapEditorPanel::Tool::BoxFill);
                ResetTilemapBoxSelection();
            }
        }

        const glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        glm::vec2 viewportMouse = {ImGui::GetMousePos().x - m_ViewportBounds[0].x,
                                   ImGui::GetMousePos().y - m_ViewportBounds[0].y};
        viewportMouse.y = viewportSize.y - viewportMouse.y;

        const float paintPlaneZ = m_EditorScene->GetEntityWorldTranslation(selectedEntity).z;
        const glm::vec3 worldPosition = TilemapEditorUtility::ViewportMouseToWorldOnPlane(
                viewportMouse, viewportSize, m_EditorCamera.GetViewProjection(), paintPlaneZ);

        const glm::mat4 inverseEntityTransform =
                glm::inverse(m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity));
        const glm::vec3 localPosition = inverseEntityTransform * glm::vec4(worldPosition, 1.0f);

        const glm::ivec2 tileCoordinates =
                TilemapEditorUtility::LocalPositionToTileCoordinates(glm::vec2(localPosition), *mapData);

        m_TilemapHoveredTile = tileCoordinates;
        m_TileMapEditorPanel.SetHoveredTileCoordinates(tileCoordinates);

        const bool validTileCoordinate = TileMapCoordinateUtility::IsValidTileCoordinate(tileCoordinates);

        if (!m_TilemapViewportCapture && !m_ViewportHovered)
            return;

        if (ImGuizmo::IsUsing())
            return;

        if (m_TilemapBoxSelecting && !m_TileMapEditorPanel.IsBoxToolActive())
            ResetTilemapBoxSelection();

        if (!allowPainting)
        {
            if (m_TilemapBoxSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                ResetTilemapBoxSelection();
            return;
        }

        const TileMapEditorPanel::Tool currentTool = m_TileMapEditorPanel.GetCurrentTool();
        if (m_TileMapEditorPanel.IsBoxTool(currentTool))
        {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && validTileCoordinate)
            {
                m_TilemapBoxSelecting = true;
                m_TilemapBoxSelectionStart = tileCoordinates;
                m_TilemapBoxSelectionEnd = tileCoordinates;
            }

            if (m_TilemapBoxSelecting)
            {
                if (validTileCoordinate)
                    m_TilemapBoxSelectionEnd = tileCoordinates;

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    if (TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapBoxSelectionStart)
                        && TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapBoxSelectionEnd))
                    {
                        m_TileMapEditorPanel.ApplyBoxTool(m_TilemapBoxSelectionStart.x,
                                                          m_TilemapBoxSelectionStart.y,
                                                          m_TilemapBoxSelectionEnd.x,
                                                          m_TilemapBoxSelectionEnd.y);
                    }
                    ResetTilemapBoxSelection();
                }
            }
            return;
        }

        if (!validTileCoordinate)
            return;

        if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_TileMapEditorPanel.ApplyToolAtTile(tileCoordinates.x, tileCoordinates.y);
    }

    void EditorLayer::ResetTilemapBoxSelection()
    {
        m_TilemapBoxSelecting = false;
        m_TilemapBoxSelectionStart = {TileMapCoordinateUtility::InvalidTileCoordinate,
                                      TileMapCoordinateUtility::InvalidTileCoordinate};
        m_TilemapBoxSelectionEnd = {TileMapCoordinateUtility::InvalidTileCoordinate,
                                    TileMapCoordinateUtility::InvalidTileCoordinate};
    }

    void EditorLayer::DrawTilemapBoxSelectionOverlay(Entity selectedEntity, float cellSize)
    {
        if (!selectedEntity || !m_EditorScene)
            return;

        if (!m_TilemapBoxSelecting)
            return;

        if (!TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapBoxSelectionStart)
            || !TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapBoxSelectionEnd))
            return;

        int32_t minTileX = m_TilemapBoxSelectionStart.x;
        int32_t maxTileX = m_TilemapBoxSelectionEnd.x;
        int32_t minTileY = m_TilemapBoxSelectionStart.y;
        int32_t maxTileY = m_TilemapBoxSelectionEnd.y;

        if (minTileX > maxTileX)
            std::swap(minTileX, maxTileX);
        if (minTileY > maxTileY)
            std::swap(minTileY, maxTileY);

        const glm::mat4 entityTransform = m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity);
        const glm::vec2 selectionBottomLeft =
                TileMapCoordinateUtility::TileLocalBottomLeft(minTileX, minTileY, cellSize);
        const glm::vec2 selectionTopRight = TileMapCoordinateUtility::TileLocalBottomLeft(
                maxTileX + 1, maxTileY + 1, cellSize);

        const bool isBoxErase =
                m_TileMapEditorPanel.GetCurrentTool() == TileMapEditorPanel::Tool::BoxErase;
        const glm::vec4 fillColor =
                isBoxErase ? glm::vec4(1.0f, 0.35f, 0.35f, 0.22f) : glm::vec4(0.35f, 0.9f, 0.45f, 0.22f);
        const glm::vec4 borderColor =
                isBoxErase ? glm::vec4(1.0f, 0.45f, 0.45f, 0.85f) : glm::vec4(0.45f, 1.0f, 0.55f, 0.85f);

        const glm::vec3 selectionCenter = {
                (selectionBottomLeft.x + selectionTopRight.x) * 0.5f,
                (selectionBottomLeft.y + selectionTopRight.y) * 0.5f,
                0.04f};
        const glm::vec3 selectionScale = {
                selectionTopRight.x - selectionBottomLeft.x,
                selectionTopRight.y - selectionBottomLeft.y,
                1.0f};

        glm::mat4 selectionTransform = entityTransform
                * glm::translate(glm::mat4(1.0f), selectionCenter)
                * glm::scale(glm::mat4(1.0f), selectionScale);

        Renderer2D::DrawRect(selectionTransform, fillColor);

        const glm::vec3 cornerBottomLeft = entityTransform
                * glm::vec4(selectionBottomLeft.x, selectionBottomLeft.y, 0.04f, 1.0f);
        const glm::vec3 cornerBottomRight = entityTransform
                * glm::vec4(selectionTopRight.x, selectionBottomLeft.y, 0.04f, 1.0f);
        const glm::vec3 cornerTopRight =
                entityTransform * glm::vec4(selectionTopRight.x, selectionTopRight.y, 0.04f, 1.0f);
        const glm::vec3 cornerTopLeft = entityTransform
                * glm::vec4(selectionBottomLeft.x, selectionTopRight.y, 0.04f, 1.0f);
        Renderer2D::DrawLine(cornerBottomLeft, cornerBottomRight, borderColor);
        Renderer2D::DrawLine(cornerBottomRight, cornerTopRight, borderColor);
        Renderer2D::DrawLine(cornerTopRight, cornerTopLeft, borderColor);
        Renderer2D::DrawLine(cornerTopLeft, cornerBottomLeft, borderColor);
    }

    void EditorLayer::DrawTilemapGhostPreviewInViewport()
    {
        if (m_SceneState != SceneState::Edit || !TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapHoveredTile)
            || !IsTilemapPaintActive())
            return;

        Entity selectedEntity;
        Ref<TileMapData> mapData;
        const TransformComponent *transformComponent = nullptr;
        if (!TryGetTilemapPaintContext(selectedEntity, mapData, transformComponent))
            return;

        const uint16_t selectedTileIdentifier = m_TileMapEditorPanel.GetSelectedTileID();
        if (selectedTileIdentifier == 0)
            return;

        if (m_TileMapEditorPanel.GetCurrentTool() == TileMapEditorPanel::Tool::Eraser
            || m_TileMapEditorPanel.GetCurrentTool() == TileMapEditorPanel::Tool::BoxErase
            || m_TileMapEditorPanel.IsBoxToolActive())
            return;

        Ref<TileSet> tileSet = m_TileMapEditorPanel.GetTileSet();
        if (!tileSet)
            return;

        const TileDef *tileDefinition = tileSet->GetTileDef(selectedTileIdentifier);
        if (!tileDefinition || tileDefinition->SourceType != TileSourceType::Atlas)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        const auto &atlasSources = tileSet->GetAtlasSources();
        if (tileDefinition->AtlasSourceIndex >= atlasSources.size())
            return;

        const TileAtlasSource &atlasSource = atlasSources[tileDefinition->AtlasSourceIndex];
        Ref<Texture2D> atlasTexture =
                std::static_pointer_cast<Texture2D>(assetManager->GetAsset(atlasSource.TextureHandle));
        if (!atlasTexture)
            return;

        const float cellSize = mapData->GetCellSize();
        const glm::vec2 tileBottomLeft = TileMapCoordinateUtility::TileLocalBottomLeft(
                m_TilemapHoveredTile.x, m_TilemapHoveredTile.y, cellSize);

        const glm::mat4 entityTransform = m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity);
        const glm::vec3 worldBottomLeft =
                entityTransform * glm::vec4(tileBottomLeft.x, tileBottomLeft.y, 0.06f, 1.0f);
        const glm::vec3 worldBottomRight =
                entityTransform * glm::vec4(tileBottomLeft.x + cellSize, tileBottomLeft.y, 0.06f, 1.0f);
        const glm::vec3 worldTopLeft =
                entityTransform * glm::vec4(tileBottomLeft.x, tileBottomLeft.y + cellSize, 0.06f, 1.0f);
        const glm::vec3 worldTopRight = entityTransform
                * glm::vec4(tileBottomLeft.x + cellSize, tileBottomLeft.y + cellSize, 0.06f, 1.0f);

        const glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        const glm::vec2 viewportOrigin = m_ViewportBounds[0];
        const glm::mat4 viewProjection = m_EditorCamera.GetViewProjection();

        const glm::vec2 screenBottomLeft =
                TilemapEditorUtility::WorldToViewportScreen(worldBottomLeft, viewportSize, viewportOrigin, viewProjection);
        const glm::vec2 screenBottomRight = TilemapEditorUtility::WorldToViewportScreen(
                worldBottomRight, viewportSize, viewportOrigin, viewProjection);
        const glm::vec2 screenTopLeft =
                TilemapEditorUtility::WorldToViewportScreen(worldTopLeft, viewportSize, viewportOrigin, viewProjection);
        const glm::vec2 screenTopRight =
                TilemapEditorUtility::WorldToViewportScreen(worldTopRight, viewportSize, viewportOrigin, viewProjection);

        const float textureWidth = (float)atlasTexture->GetWidth();
        const float textureHeight = (float)atlasTexture->GetHeight();
        const float tilePixelSize = (float)atlasSource.TileSize;
        if (tilePixelSize <= 0.0f)
            return;

        const glm::ivec4 pixelRect = SpriteSheetUtility::AtlasGridCoordsToPixelRect(
                tileDefinition->AtlasCoords, static_cast<uint32_t>(tilePixelSize));
        const std::array<glm::vec2, 4> imguiQuadUVs =
                SpriteSheetUtility::PixelRectToImGuiQuadUVsForScreenCorners(
                        pixelRect, atlasTexture->GetWidth(), atlasTexture->GetHeight());

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        drawList->AddImageQuad((ImTextureID)(uint64_t)atlasTexture->GetRendererID(),
                               ImVec2(screenBottomLeft.x, screenBottomLeft.y),
                               ImVec2(screenBottomRight.x, screenBottomRight.y),
                               ImVec2(screenTopRight.x, screenTopRight.y),
                               ImVec2(screenTopLeft.x, screenTopLeft.y),
                               ImVec2(imguiQuadUVs[0].x, imguiQuadUVs[0].y),
                               ImVec2(imguiQuadUVs[1].x, imguiQuadUVs[1].y),
                               ImVec2(imguiQuadUVs[2].x, imguiQuadUVs[2].y),
                               ImVec2(imguiQuadUVs[3].x, imguiQuadUVs[3].y),
                               IM_COL32(255, 255, 255, 180));
    }

    void EditorLayer::DrawTilemapEditOverlay()
    {
        if (m_SceneState != SceneState::Edit)
            return;

        Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
        if (!selectedEntity || !selectedEntity.HasComponent<TilemapComponent>()
            || !selectedEntity.HasComponent<TransformComponent>())
            return;

        Ref<TileMapData> mapData = m_TileMapEditorPanel.GetMapData();
        if (!mapData)
            return;

        const glm::mat4 entityTransform = m_EditorScene->GetEntityWorldTransformMatrix(selectedEntity);

        const float cellSize = mapData->GetCellSize();
        const TileMapEditorGridBounds gridBounds =
                TileMapCoordinateUtility::GetEditorGridBounds(*mapData);

        const glm::vec4 gridColor = {0.4f, 0.8f, 1.0f, 0.35f};
        const glm::vec4 borderColor = {0.9f, 0.9f, 0.2f, 0.7f};

        const glm::vec2 gridBottomLeft = TileMapCoordinateUtility::TileLocalBottomLeft(
                gridBounds.MinTileX, gridBounds.MinTileY, cellSize);
        const glm::vec2 gridTopRight = TileMapCoordinateUtility::TileLocalBottomLeft(
                gridBounds.MaxTileX + 1, gridBounds.MaxTileY + 1, cellSize);

        for (int32_t gridX = gridBounds.MinTileX; gridX <= gridBounds.MaxTileX + 1; ++gridX)
        {
            const float localX = TileMapCoordinateUtility::TileLocalBottomLeft(gridX, 0, cellSize).x;
            const glm::vec3 start = entityTransform * glm::vec4(localX, gridBottomLeft.y, 0.02f, 1.0f);
            const glm::vec3 end = entityTransform * glm::vec4(localX, gridTopRight.y, 0.02f, 1.0f);
            Renderer2D::DrawLine(start, end, gridColor);
        }

        for (int32_t gridY = gridBounds.MinTileY; gridY <= gridBounds.MaxTileY + 1; ++gridY)
        {
            const float localY = TileMapCoordinateUtility::TileLocalBottomLeft(0, gridY, cellSize).y;
            const glm::vec3 start = entityTransform * glm::vec4(gridBottomLeft.x, localY, 0.02f, 1.0f);
            const glm::vec3 end = entityTransform * glm::vec4(gridTopRight.x, localY, 0.02f, 1.0f);
            Renderer2D::DrawLine(start, end, gridColor);
        }

        const glm::vec3 cornerBottomLeft =
                entityTransform * glm::vec4(gridBottomLeft.x, gridBottomLeft.y, 0.02f, 1.0f);
        const glm::vec3 cornerBottomRight =
                entityTransform * glm::vec4(gridTopRight.x, gridBottomLeft.y, 0.02f, 1.0f);
        const glm::vec3 cornerTopRight =
                entityTransform * glm::vec4(gridTopRight.x, gridTopRight.y, 0.02f, 1.0f);
        const glm::vec3 cornerTopLeft =
                entityTransform * glm::vec4(gridBottomLeft.x, gridTopRight.y, 0.02f, 1.0f);

        Renderer2D::DrawLine(cornerBottomLeft, cornerBottomRight, borderColor);
        Renderer2D::DrawLine(cornerBottomRight, cornerTopRight, borderColor);
        Renderer2D::DrawLine(cornerTopRight, cornerTopLeft, borderColor);
        Renderer2D::DrawLine(cornerTopLeft, cornerBottomLeft, borderColor);

        DrawTilemapBoxSelectionOverlay(selectedEntity, cellSize);

        if (TileMapCoordinateUtility::IsValidTileCoordinate(m_TilemapHoveredTile)
            && !m_TilemapBoxSelecting && !m_TileMapEditorPanel.IsBoxToolActive())
        {
            const glm::vec2 tileCenter = TileMapCoordinateUtility::TileLocalCenter(
                    m_TilemapHoveredTile.x, m_TilemapHoveredTile.y, cellSize);

            glm::mat4 highlightTransform = entityTransform
                    * glm::translate(glm::mat4(1.0f), glm::vec3(tileCenter.x, tileCenter.y, 0.05f))
                    * glm::scale(glm::mat4(1.0f), glm::vec3(cellSize * 0.98f, cellSize * 0.98f, 1.0f));

            const glm::vec4 highlightColor = {0.3f, 0.85f, 1.0f, 0.25f};
            Renderer2D::DrawRect(highlightTransform, highlightColor);

            const uint16_t selectedTileIdentifier = m_TileMapEditorPanel.GetSelectedTileID();
            if (IsTilemapPaintActive() && selectedTileIdentifier != 0
                && m_TileMapEditorPanel.GetCurrentTool() != TileMapEditorPanel::Tool::Eraser
                && m_TileMapEditorPanel.GetCurrentTool() != TileMapEditorPanel::Tool::BoxErase)
            {
                Ref<TileSet> tileSet = m_TileMapEditorPanel.GetTileSet();
                if (tileSet)
                {
                    const TileDef *tileDefinition = tileSet->GetTileDef(selectedTileIdentifier);
                    if (tileDefinition && tileDefinition->SourceType == TileSourceType::Atlas)
                    {
                        const auto &atlasSources = tileSet->GetAtlasSources();
                        if (tileDefinition->AtlasSourceIndex < atlasSources.size())
                        {
                            auto assetManager = ResourceSystem::GetAssetManager();
                            if (assetManager)
                            {
                                const TileAtlasSource &atlasSource =
                                        atlasSources[tileDefinition->AtlasSourceIndex];
                                Ref<Texture2D> atlasTexture = std::static_pointer_cast<Texture2D>(
                                        assetManager->GetAsset(atlasSource.TextureHandle));
                                if (atlasTexture && atlasSource.TileSize > 0)
                                {
                                    const std::array<glm::vec2, 4> textureCoordinates =
                                            SpriteSheetUtility::AtlasGridCoordsToWorldQuadUVs(
                                                    tileDefinition->AtlasCoords, atlasSource.TileSize,
                                                    atlasTexture->GetWidth(), atlasTexture->GetHeight());

                                    glm::mat4 ghostTransform = entityTransform
                                            * glm::translate(glm::mat4(1.0f),
                                                             glm::vec3(tileCenter.x, tileCenter.y, 0.06f))
                                            * glm::scale(glm::mat4(1.0f),
                                                         glm::vec3(cellSize * 0.98f, cellSize * 0.98f, 1.0f));

                                    Renderer2D::DrawQuadUV(ghostTransform, atlasTexture, textureCoordinates, 1.0f,
                                                          glm::vec4(1.0f, 1.0f, 1.0f, 0.55f));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

} // namespace Himii
