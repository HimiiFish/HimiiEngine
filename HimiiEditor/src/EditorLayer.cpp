#include "EditorLayer.h"
#include "imgui.h"
#include "Himii/Scripting/ScriptEngine.h"
#include "Himii/Project/Project.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "CamerController.h"
#include "Himii/Utils/PlatformUtils.h"
#include "ImGuizmo.h"
#include "yaml-cpp/yaml.h"

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

        LoadRecentProjects();

        auto commandLineArgs = Application::Get().GetCommandLineArgs();
        if (commandLineArgs.Count > 1)
        {
            OpenProject(commandLineArgs.Args[1]);
        }
        else
        {
            // 2. 开发模式：如果有 HIMII_ROOT_DIR 且用户没有特别设置，是否自动打开？
            // 为了展示 Hub，这里修改为：如果不强制要求，默认不打开。
            // 如果确实需要自动打开用于调试，可以取消下面的注释，或者加个 Engine 配置。
/*
#ifdef HIMII_ROOT_DIR
            // 自动打开源码目录下的测试项目
            std::filesystem::path root(HIMII_ROOT_DIR);
            std::filesystem::path sandboxProject = root / "Sandbox" / "Sandbox.hproj"; 
            if (std::filesystem::exists(sandboxProject))
                OpenProject(sandboxProject);
#endif
*/
            // 保持无 Project 状态，以显示 Hub
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
                m_EditorCamera.OnUpdate(ts, m_ViewportHovered);
                m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);
                break;
            }
            case SceneState::Play:
            {
                m_ActiveScene->OnUpdateRuntime(ts);
                break;
            }
            case SceneState::Simulate:
                m_EditorCamera.OnUpdate(ts, m_ViewportHovered);
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

        // 如果没有 Active Project，显示 Project Hub
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
                    if (ImGui::MenuItem("Save Project"))
                    {
                        SaveProject();
                    }
                    if (ImGui::MenuItem("Build Project..."))
                    {
                        BuildProject();
                    }
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
                if (ImGui::BeginMenu("Window"))
                {
                    ImGui::MenuItem("Animation Editor", nullptr, &m_ShowAnimationPanel);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            m_SceneHierarchyPanel.OnImGuiRender();
            m_ContentBrowserPanel.OnImGuiRender();
            m_AnimationPanel.OnImGuiRender(m_ShowAnimationPanel);

            

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

                // Only enable gizmo if viewport is hovered or we are already using it
                ImGuizmo::Enable(m_ViewportHovered || ImGuizmo::IsUsing());
                
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
        if (m_ViewportHovered)
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

    void EditorLayer::BuildProject()
    {
        // 1. 让用户选择输出路径 (这里复用 SaveFile，让用户输入 exe 名字)
        std::string filepath = FileDialog::SaveFile("Executable (*.exe)\0*.exe\0");

        if (filepath.empty())
            return;

        std::filesystem::path exePath(filepath);
        std::filesystem::path buildDir = exePath.parent_path();

        // 如果目录不存在，创建它
        if (!std::filesystem::exists(buildDir))
            std::filesystem::create_directories(buildDir);

        HIMII_CORE_INFO("Starting Build to: {0}", buildDir.string());

        // 2. 确定源文件位置 (Editor 和 Runtime 通常在同一个构建目录下)
        // 获取当前 Editor.exe 所在的目录
        std::filesystem::path engineBinDir = Application::GetEngineDir();

        std::filesystem::path binRoot = (engineBinDir / "../../../").lexically_normal();
        std::filesystem::path runtimeDir = binRoot / "HimiiRuntime" / "Debug";

        // 3. 定义需要复制的文件清单
        struct CopyEntry {
            std::filesystem::path Source;
            std::filesystem::path Dest;
            bool IsDirectory = false;
        };

        std::vector<CopyEntry> filesToCopy;

        // A. 复制 Runtime 可执行文件 (HimiiRuntime.exe -> 用户指定的 MyGame.exe)
        filesToCopy.push_back({runtimeDir/"HimiiRuntime.exe", exePath});

        filesToCopy.push_back({runtimeDir / "ScriptCore.runtimeconfig.json", buildDir / "ScriptCore.runtimeconfig.json"});

        if (std::filesystem::exists(runtimeDir))
        {
            for (auto &entry: std::filesystem::directory_iterator(runtimeDir))
            {
                // 只要是 .dll 文件，统统复制过去
                if (entry.path().extension() == ".dll")
                {
                    // 排除掉可能存在的旧 GameAssembly (我们后面会从项目目录复制最新的)
                    if (entry.path().stem() == "GameAssembly")
                        continue;

                    filesToCopy.push_back({entry.path(), buildDir / entry.path().filename()});
                }
            }
        }
        else
        {
            HIMII_CORE_ERROR("Runtime directory not found for DLL scanning!");
        }

        std::filesystem::path engineAssetsDir = engineBinDir / "assets";

        if (std::filesystem::exists(engineAssetsDir))
        {
            HIMII_CORE_INFO("Copying Engine Assets from: {0}", engineAssetsDir.string());
            // 复制引擎资源作为“默认值”
            filesToCopy.push_back({engineAssetsDir, buildDir / "assets", true});
        }
        else
        {
            HIMII_CORE_WARNING("Could not locate Engine Assets (shaders)! Runtime might crash.");
        }

        //复制 Game DLL
        std::filesystem::path gameDllSource = Project::GetProjectDirectory() / Project::GetConfig().ScriptModulePath;
        filesToCopy.push_back({gameDllSource, buildDir / "GameAssembly.dll"}); // 强制改名或保持原名

        //复制 Assets 文件夹 (递归)
        filesToCopy.push_back({Project::GetAssetDirectory(), buildDir / "assets", true});

        //复制并重命名项目配置文件 (.hproj)
        std::filesystem::path projectSource = Project::GetProjectDirectory() / (Project::GetConfig().Name + ".hproj");
        filesToCopy.push_back({projectSource, buildDir / "Game.hproj"});


        int successCount = 0;
        for (auto &entry: filesToCopy)
        {
            try
            {
                if (std::filesystem::exists(entry.Source))
                {
                    if (entry.IsDirectory)
                    {
                        std::filesystem::copy(entry.Source, entry.Dest,
                                              std::filesystem::copy_options::recursive |
                                                      std::filesystem::copy_options::overwrite_existing);
                    }
                    else
                    {
                        std::filesystem::copy_file(entry.Source, entry.Dest,
                                                   std::filesystem::copy_options::overwrite_existing);
                    }
                    successCount++;
                }
                else
                {
                    // 只有关键文件缺失才报错
                    HIMII_CORE_WARNING("Build Warning: Source file missing {0}", entry.Source.string());
                }
            }
            catch (std::exception &e)
            {
                HIMII_CORE_ERROR("Build Failed: {0}", e.what());
            }
        }

        if (successCount > 0)
            HIMII_CORE_INFO("Build Completed Successfully! Output: {0}", buildDir.string());
        else
            HIMII_CORE_ERROR("Build Failed - No files copied.");
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
            // 尝试1: 环境变量中的 code
            // 使用 where code (Windows)检测是否存在
            bool hasCodeInPath = system("where code >nul 2>nul") == 0;

            std::string cmd;
            if (hasCodeInPath) 
            {
                 cmd = "code \"" + projectDir.string() + "\"";
            }
            else
            {
                // 尝试2: 常见安装路径
                std::vector<std::string> vscodePaths = {
                    "\"C:\\Users\\" + std::string(getenv("USERNAME")) + "\\AppData\\Local\\Programs\\Microsoft VS Code\\bin\\code.cmd\"",
                    "\"C:\\Program Files\\Microsoft VS Code\\bin\\code.cmd\"",
                    "\"C:\\Program Files (x86)\\Microsoft VS Code\\bin\\code.cmd\""
                };
                
                bool found = false;
                for(const auto& path : vscodePaths) {
                    // 去掉引号判断是否存在
                    std::string rawPath = path.substr(1, path.length() - 2);
                    if(std::filesystem::exists(rawPath)) {
                        cmd = path + " \"" + projectDir.string() + "\"";
                        found = true;
                        break;
                    }
                }

                if(!found) {
                     // 实在找不到，只能尝试 explorer 打开文件夹，让用户自己拖
                     HIMII_CORE_ERROR("VS Code not found in PATH or default locations.");
                     system(("explorer \"" + projectDir.string() + "\"").c_str());
                     return;
                }
            }
            
            system(cmd.c_str());
            HIMII_CORE_INFO("Opening C# Project: {0}", projectDir.string());
        }
        else
        {
            HIMII_CORE_WARNING("Solution file not found! Please regenerate project.");
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

        // Center content
        // ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 600) * 0.5f);
        
        ImGui::Text("Himii Engine - Project Hub");
        ImGui::Separator();
        
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 300.0f);

        // Left Column: Actions
        if (ImGui::Button("Create New Project", ImVec2(280, 50)))
        {
            NewProject();
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
