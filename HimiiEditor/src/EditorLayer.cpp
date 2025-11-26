#include "EditorLayer.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "CamerController.h"
#include "Himii/Utils/PlatformUtils.h"
#include "ImGuizmo.h"

namespace Himii
{
    EditorLayer::EditorLayer() :
        Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
    {
    }

    void EditorLayer::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        m_ActiveScene = CreateRef<Scene>();

        // 创建离屏帧缓冲，尺寸先用窗口大小，后续由 EditorLayer 面板驱动调整
        FramebufferSpecification fbSpec{1280, 720};
        fbSpec.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::RED_INTEGER, FramebufferFormat::Depth};
        m_Framebuffer = Framebuffer::Create(fbSpec);

        m_EditorCamera = EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);

        //{
        //    m_SquareEntity = m_ActiveScene->CreateEntity("My Quad");
        //    m_SquareEntity.AddComponent<SpriteRendererComponent>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
        //    m_SquareEntity.AddComponent<NativeScriptComponent>().Bind<CameraController>();

        //    for (int i = -5; i < 10; i++)
        //    {
        //        auto m_Entity = m_ActiveScene->CreateEntity("Entity");
        //        m_Entity.AddComponent<SpriteRendererComponent>(glm::vec4{0.2f, 0.3f, 0.8f, 1.0f});
        //        m_Entity.GetComponent<TransformComponent>().Position = {i * 1.1f, 0.0f, 0.0f};
        //    }

        //    m_CameraEntity = m_ActiveScene->CreateEntity("Camera Entity");
        //    m_CameraEntity.AddComponent<CameraComponent>();
        //    // 默认构造 SpriteRenderer（白色），或传入颜色
        //}
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
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

        m_EditorCamera.OnUpdate(ts);
        if (m_ViewportFocused)
        {
        }

        // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
        Renderer2D::ResetStats();

        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        RenderCommand::Clear();

        m_Framebuffer->ClearAttachment(1, -1); // 1号附件清除为 -1（无实体）


        HIMII_PROFILE_SCOPE("Renderer Draw");
        m_ActiveScene->OnUpdateEditor(ts, m_EditorCamera);

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
            HIMII_CORE_INFO("pixel data = {0}", pixelData);
            m_HoveredEntity = pixelData == -1 ? Entity() : Entity((entt::entity)pixelData, m_ActiveScene.get());
        }

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
                if (ImGui::BeginMenu("Options"))
                {
                    if (ImGui::MenuItem("New..", "Ctrl+N"))
                    {
                        NewScene();
                    }

                    if (ImGui::MenuItem("Open..", "Ctrl+O"))
                    {
                        OpenScene();
                    }

                    if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S"))
                    {
                        SaveSceneAs();
                    }

                    if (ImGui::MenuItem("Quit"))
                        Himii::Application::Get().Close();
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            m_SceneHierarchyPanel.OnImGuiRender();

            ImGui::Begin("Stats");
            auto stats = Himii::Renderer2D::GetStatistics();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quad Count: %d", stats.QuadCount);
            ImGui::Text("Vertex Count: %d", stats.GetTotalVertexCount());
            ImGui::Text("Index Count: %d", stats.GetTotalIndexCount());
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
                float snapValue = 0.5f; // translation snap
                if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                    snapValue = 45.0f; // rotation snap
                else if (m_GizmoType == ImGuizmo::OPERATION::SCALE)
                    snapValue = 0.5f; // scale snap
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
                    HIMII_CORE_INFO("保存场景");
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

    void EditorLayer::NewScene()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OpenScene()
    {
        std::string filePath = FileDialog::OpenFile("Himii Scene(*.himii)\0*.himii\0");

        if (!filePath.empty())
        {
            m_ActiveScene = CreateRef<Scene>();
            SceneSerializer serializer(m_ActiveScene);
            serializer.Deserialize(filePath);
            // m_ActiveScene->Clear();
            m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_SceneHierarchyPanel.SetContext(m_ActiveScene);
        }
    }

    void EditorLayer::SaveScene()
    {
        std::string filePath = FileDialog::SaveFile("Himii Scene(*.himii)\0*.himii\0");
        if (!filePath.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.Serialize(filePath);
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        std::string filePath = FileDialog::SaveFile("Himii Scene(*.himii)\0*.himii\0");
        if (!filePath.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.Serialize(filePath);
        }
    }
} // namespace Himii
