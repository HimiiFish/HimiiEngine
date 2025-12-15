#include "EditorLayer.h"
#include "imgui.h"
#include "Himii/Scripting/ScriptEngine.h"
#include "Himii/Project/Project.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "CamerController.h"
#include "Himii/Utils/PlatformUtils.h"
#include "ImGuizmo.h"

namespace Himii
{
    //extern const std::filesystem::path s_AssetsPath;

    EditorLayer::EditorLayer() :
        Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
    {
    }

    void EditorLayer::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        m_IconPlay = Texture2D::Create("resources/icons/play.png");
        m_IconStop = Texture2D::Create("resources/icons/stop.png");
        m_IconSimulate = Texture2D::Create("resources/icons/simulate.png");

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;


        // 创建离屏帧缓冲，尺寸先用窗口大小，后续由 EditorLayer 面板驱动调整
        FramebufferSpecification fbSpec{1280, 720};
        fbSpec.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::RED_INTEGER, FramebufferFormat::Depth};
        m_Framebuffer = Framebuffer::Create(fbSpec);

        m_EditorCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 1)
        {
            OpenProject(commandLineArgs.Args[1]);
        }
        else
        {
            // 2. 开发模式：如果有 HIMII_ROOT_DIR，自动打开 Sandbox
#ifdef HIMII_ROOT_DIR
            // 自动打开源码目录下的测试项目
            std::filesystem::path root(HIMII_ROOT_DIR);
            std::filesystem::path sandboxProject = root / "Sandbox" / "Sandbox.hproj"; // 假设你有这个
            if (std::filesystem::exists(sandboxProject))
                OpenProject(sandboxProject);
#endif
            // 如果都不是，什么都不做，ContentBrowser 会显示 "Please open project"
        }
    }
    void EditorLayer::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        HIMII_PROFILE_FUNCTION();

        // m_CameraController.OnUpdate(ts);

        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
            (spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
        {
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
            m_ActiveScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        }

        // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
        Renderer2D::ResetStats();

        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        RenderCommand::Clear();

        m_Framebuffer->ClearAttachment(1, -1); // 1号附件清除为 -1（无实体）

        switch (m_SceneState)
        {
            case SceneState::Edit:
            {
                m_EditorCamera.OnUpdate(ts);
                m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
                break;
            }
            case SceneState::Play:
            {
                m_ActiveScene->OnUpdateRuntime(ts);
                break;
            }
            case SceneState::Simulate:
                m_EditorCamera.OnUpdate(ts);
                m_ActiveScene->OnUpdateSimulation(ts, m_EditorCamera);
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
            m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
        }

        OnOverlayRender();

        m_Framebuffer->Unbind();
    }
    void EditorLayer::OnImGuiRender()
    {
        HIMII_PROFILE_FUNCTION();

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
                ImGui::SetNextWindowPos(viewport->Pos);
                ImGui::SetNextWindowSize(viewport->Size);
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

            // DockSpace
            ImGuiIO &io = ImGui::GetIO();
            ImGuiStyle &style = ImGui::GetStyle();
            float minWinSizeX = style.WindowMinSize.x;
            style.WindowMinSize.x = 370.0f;
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }

            style.WindowMinSize.x = minWinSizeX;

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    // --- 新建项目 ---
                    if (ImGui::MenuItem("New Project..."))
                    {
                        NewProject();
                    }

                    // --- 打开项目 ---
                    if (ImGui::MenuItem("Open Project..."))
                    {
                        std::string filepath = FileDialog::OpenFile("Himii Project (*.hproj)\0*.hproj\0");
                        if (!filepath.empty())
                            OpenProject(filepath);
                    }
                    // ...
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Options"))
                {
                    if (ImGui::MenuItem("New Scene..", "Ctrl+N"))
                    {
                        NewScene();
                    }

                    if (ImGui::MenuItem("Open Scene..", "Ctrl+O"))
                    {
                        OpenScene();
                    }

                    if (ImGui::MenuItem("Save Scene As..", "Ctrl+Shift+S"))
                    {
                        SaveSceneAs();
                    }

                    if (ImGui::MenuItem("Quit"))
                        Himii::Application::Get().Close();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Script"))
                {
                    if (ImGui::MenuItem("Compile and Reload"))
                    {
                        CompileAndReloadScripts();
                    }
                    if (ImGui::MenuItem("Open C# Solution"))
                    {
                        OpenCSProject();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            m_SceneHierarchyPanel.OnImGuiRender();
            m_ContentBrowserPanel.OnImGuiRender();

            ImGui::Begin("Stats");
            auto stats = Himii::Renderer2D::GetStatistics();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quad Count: %d", stats.QuadCount);
            ImGui::Text("Vertex Count: %d", stats.GetTotalVertexCount());
            ImGui::Text("Index Count: %d", stats.GetTotalIndexCount());
            ImGui::End();

            ImGui::Begin("Settings");
            ImGui::Checkbox("Show physics colliders", &m_ShowPhysicsColliders);

            //ImGui::Image((ImTextureID)s_Font->GetAtlasTexture()->GetRendererID(), {512, 512}, {0, 1}, {1, 0});

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
            Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if (m_ViewportSize != *((glm::vec2 *)&viewportPanelSize))
            {
                m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
            }

            uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image(reinterpret_cast<void *>(textureID), ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1),
                         ImVec2(1, 0));

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    const wchar_t *path = (const wchar_t *)payload->Data;
                    OpenScene(std::filesystem::path(Project::GetAssetDirectory()) / path);
                }

                ImGui::EndDragDropTarget();
            }
            // gizmos
            Entity selectEntity = m_SceneHierarchyPanel.GetSelectedEntity();
            if (selectEntity && m_GizmoType != -1)
            {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();

                float windowWidth = ImGui::GetWindowWidth();
                float windowHeight = ImGui::GetWindowHeight();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

                // runtime camera
                /*auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
                const auto &camera = cameraEntity.GetComponent<CameraComponent>().camera;
                const glm::mat4 &cameraProjection = camera.GetProjection();
                glm::mat4 cameraView = glm::inverse(cameraEntity.GetComponent<TransformComponent>().GetTransform());*/

                // editor camera
                const glm::mat4 &cameraProjection = m_EditorCamera.GetProjection();
                glm::mat4 cameraView = m_EditorCamera.GetViewMatrix();

                // Entity transform
                auto &transformComponent = selectEntity.GetComponent<TransformComponent>();
                glm::mat4 transform = transformComponent.GetTransform();

                // snapping
                bool snap = Input::IsKeyPressed(Key::LeftControl);
                float snapValue = 0.5f; // Snap to 0.5m for translation/scale
                // Snap to 45 degrees for rotation
                if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                    snapValue = 45.0f;
                float snapValues[3] = {snapValue, snapValue, snapValue};

                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                     (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(transform),
                                     nullptr, snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing())
                {
                    glm::vec3 translation, rotation, scale;
                    Math::DecomposeTransform(transform, translation, rotation, scale);

                    glm::vec3 deltaRotation = rotation - transformComponent.Rotation;
                    transformComponent.Position = translation;
                    transformComponent.Rotation += deltaRotation;
                    transformComponent.Scale = scale;
                }
            }

            // ImGui::End();
            ImGui::PopStyleVar();

            UI_Toolbar();

            ImGui::End();
        }
    }

    void EditorLayer::OnEvent(Himii::Event &event)
    {
        m_CameraController.OnEvent(event);
        m_EditorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
    {
        if (e.IsRepeat() > 0)
            return false;
        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightControl);
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
                    SaveScene();
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
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = -1;
                break;
            }
            case Key::W:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            }
            case Key::E:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            }
            case Key::R:
            {
                if (!ImGuizmo::IsUsing())
                    m_GizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            }
            case Key::Delete:
            {
                if (Application::Get().GetImGuiLayer()->GetActiveWidgetID() == 0)
				{
					Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();
					if (selectedEntity)
					{
						m_SceneHierarchyPanel.SetSelectedEntity({});
						m_ActiveScene->DestroyEntity(selectedEntity);
					}
				}
				break;
            }
        }
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
    {
        if (e.GetMouseButton() == Mouse::ButtonLeft)
        {
            if (m_ViewportHovered && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
                m_SceneHierarchyPanel.SetSelectedEntity(m_HoveredEntity);
        }
        return false;
    }

    void EditorLayer::OnOverlayRender()
    {
        if (m_SceneState == SceneState::Play)
        {
            Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
            if (!camera)
                return;

            Renderer2D::BeginScene(camera.GetComponent<CameraComponent>().Camera,
                                   camera.GetComponent<TransformComponent>().GetTransform());
        }
        else
        {
            Renderer2D::BeginScene(m_EditorCamera);
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
        }
        // Draw selected entity outline
        if (Entity selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity())
        {
            const TransformComponent &transform = selectedEntity.GetComponent<TransformComponent>();
            Renderer2D::DrawRect(transform.GetTransform(), glm::vec4(1.0f, 0.5f, 0.0f, 1.0f));
        }

        Renderer2D::EndScene();
    }

    void EditorLayer::NewProject()
    {
        std::string filepath = FileDialog::SaveFile("Himii Project (*.hproj)\0*.hproj\0");

        if (!filepath.empty())
        {
            std::filesystem::path path(filepath);

            std::filesystem::path parentDir = path.parent_path();
            std::string projectName = path.stem().string();

            // 1. 构建目录路径
            std::filesystem::path newProjectDir = parentDir / projectName;
            std::filesystem::path newProjectFilePath = newProjectDir / (projectName + ".hproj");

            // 2. 生成物理文件 (csproj 和 assets 文件夹)
            Project::CreateProjectFiles(projectName, newProjectDir);

            //创建临时场景
            Ref<Scene> startScene = CreateRef<Scene>();
            startScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            {
                Entity cameraEntity = startScene->CreateEntity("Main Camera");
                auto &cc = cameraEntity.AddComponent<CameraComponent>();
                cc.Camera.SetProjectionType(SceneCamera::ProjectionType::Orthographic);
                cameraEntity.GetComponent<TransformComponent>().Position = {0.0f, 0.0f, 10.0f};
            }

            std::filesystem::path relativeScenePath = "scenes/Start.himii";
            std::filesystem::path fullScenePath = newProjectDir / "assets" / relativeScenePath;

            SceneSerializer sceneSerializer(startScene);
            sceneSerializer.Serialize(fullScenePath.string());

            HIMII_CORE_INFO("Created default scene at {0}", fullScenePath.string());

            // 2. 在内存中配置 Project 对象
            Ref<Project> newProject = Project::New();
            newProject->GetConfig().Name = projectName;
            newProject->GetConfig().AssetDirectory = "assets"; // 相对路径
            newProject->GetConfig().StartScene = relativeScenePath;
            newProject->GetConfig().ScriptModulePath = "bin/Debug/GameAssembly.dll";

            // 3. 序列化 .hproj 文件
            Project::SaveActive(newProjectFilePath);

            // 4. 立即加载这个新项目
            OpenProject(newProjectFilePath);

            HIMII_CORE_INFO("New project created and loaded: {0}", newProjectFilePath.string());
        }
    }

    void EditorLayer::OpenProject(const std::filesystem::path &path)
    {
        if (Project::Load(path))
        {
            auto projectDir = Project::GetProjectDirectory();

            // 假设 csproj 就在项目根目录
            // 如果你的结构是 csproj 在 assets 下，则改为 projectDir / "assets" / "GameAssembly.csproj"
            m_CSharpProjectPath = projectDir /Project::GetProjectDirectory()/ "GameAssembly.csproj";

            // 编译脚本
            CompileAndReloadScripts();

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
        }
    }

    void EditorLayer::OpenCSProject()
    {
        if (!Project::GetActive())
            return;

        auto projectDir = Project::GetProjectDirectory();
        std::string projectName = Project::GetConfig().Name;

        // 方案 A: 打开 .sln (推荐，VS 和 Rider 会直接识别，VS Code 也会提示打开工作区)
        std::filesystem::path slnPath = projectDir / (projectName + ".sln");

        if (std::filesystem::exists(slnPath))
        {
            // Windows: 使用 shell execute 打开关联程序
            // system(("start \"\" \"" + slnPath.string() + "\"").c_str());
            // 为了兼容性，也可以直接调用 code

            // 方案 B (针对 VS Code 优化): 直接用 code 打开文件夹
            // 这样 VS Code 会加载文件夹下的 .sln 和 .csproj
            std::string cmd = "code \"" + projectDir.string() + "\"";
            system(cmd.c_str());

            HIMII_CORE_INFO("Opening C# Project: {0}", projectDir.string());
        }
        else
        {
            HIMII_CORE_WARNING("Solution file not found! Please regenerate project.");
            // 可以补一个 Project::CreateProjectFiles 的调用来重新生成
        }
    }

    void EditorLayer::NewScene()
    {
        m_SceneHierarchyPanel.SetSelectedEntity({});
        m_HoveredEntity = {};

        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);

        m_EditorScenePath = std::filesystem::path();
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
            m_SceneHierarchyPanel.SetContext(m_EditorScene);
            m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            m_ActiveScene = m_EditorScene;
            m_EditorScenePath = path;
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
        if (m_SceneState == SceneState::Simulate)
            OnSceneStop();

        m_SceneState = SceneState::Play;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_ActiveScene->OnRuntimeStart();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneSimulate()
    {
        if (m_SceneState == SceneState::Play)
            OnSceneStop();

        m_SceneState = SceneState::Simulate;

        m_ActiveScene = Scene::Copy(m_EditorScene);
        m_ActiveScene->OnSimulationStart();

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnSceneStop()
    {
        //HIMII_CORE_ASSERT(m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulate);

        m_SceneHierarchyPanel.SetSelectedEntity({});

        if (m_SceneState == SceneState::Play)
            m_ActiveScene->OnRuntimeStop();
        else if (m_SceneState == SceneState::Simulate)
            m_ActiveScene->OnSimulationStop();

        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;

        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
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

    void EditorLayer::CompileAndReloadScripts()
    {
        ScriptEngine::CompileAndReloadAppAssembly(m_CSharpProjectPath);
    }

    void EditorLayer::UI_Toolbar()
    {
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
            Ref<Texture2D> icon = (m_SceneState == SceneState::Edit||m_SceneState==SceneState::Simulate) ? m_IconPlay : m_IconStop;
            ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
            if (ImGui::ImageButton("state", (ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0),ImVec2(1, 1),ImVec4(0.0f,0.0f,0.0f,0.0f),tintColor)&&toolbarEnable)
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
            //ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
            if (ImGui::ImageButton("state1", (ImTextureID)icon->GetRendererID(), ImVec2(size, size), ImVec2(0, 0),ImVec2(1, 1),ImVec4(0.0f,0.0f,0.0f,0.0f),tintColor)&&toolbarEnable)
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
} // namespace Himii
