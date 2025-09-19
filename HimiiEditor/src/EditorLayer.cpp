#include "EditorLayer.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace Himii
{
    EditorLayer::EditorLayer() :
        Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
    {
    }

    void EditorLayer::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        // 创建离屏帧缓冲，尺寸先用窗口大小，后续由 EditorLayer 面板驱动调整
        auto &app = Application::Get();
        FramebufferSpecification fbSpec{app.GetWindow().GetWidth(), app.GetWindow().GetHeight()};
        fbSpec.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::RED_INTEGER,FramebufferFormat::Depth};
        m_Framebuffer = Framebuffer::Create(fbSpec);

        // 最小 ECS 场景：创建几个彩色方块实体（使用 Entity 包装）
        {
            auto e4 = m_Scene.CreateEntity("My Quad");
            // 默认构造 SpriteRenderer（白色），或传入颜色
            e4.AddComponent<SpriteRenderer>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
        }
    }
    void EditorLayer::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        HIMII_PROFILE_FUNCTION();

        m_CameraController.OnUpdate(ts);

        if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
            m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
            (spec.Width != (uint32_t)m_ViewportSize.x || spec.Height != (uint32_t)m_ViewportSize.y))
        {
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
            m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
        }

        // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
        Renderer2D::ResetStats();

        m_Framebuffer->Bind();
        RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        RenderCommand::Clear();

        m_Framebuffer->ClearAttachment(1, -1); // 1号附件清除为 -1（无实体）

        HIMII_PROFILE_SCOPE("Renderer Draw");
        Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_Scene.OnUpdate(ts);
        Renderer2D::EndScene();
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
                if (ImGui::BeginMenu("选项"))
                {
                    if (ImGui::MenuItem("退出"))
                        Himii::Application::Get().Close();
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            ImGui::End();

            ImGui::Begin("Stats");
            auto stats = Himii::Renderer2D::GetStatistics();
            ImGui::Text("Renderer2D Stats:");
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quad Count: %d", stats.QuadCount);
            ImGui::Text("Vertex Count: %d", stats.GetTotalVertexCount());
            ImGui::Text("Index Count: %d", stats.GetTotalIndexCount());
            ImGui::End();

            ImGui::Begin("ViewPort");
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if (m_ViewportSize != *((glm::vec2 *)&viewportPanelSize))
            {
                m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};
            }

            uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image((void *)textureID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
            ImGui::End();
        }
    }

    void EditorLayer::OnEvent(Himii::Event &event)
    {
        m_CameraController.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        if (e.IsRepeat() > 0)
            return false;
        bool control = Input::IsKeyPressed(Key::LeftControl) ||
                       Input::IsKeyPressed(Key::RightControl);
        bool shift =
                Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightControl);
        switch (e.GetKeyCode())
        {
            case Key::N:
            {
                if (control)
                {
                    HIMII_CORE_INFO("新建场景");
                }
                break;
            }
            case Key::O:
            {
                if (control)
                {
                    HIMII_CORE_INFO("打开场景");
                }
                break;
            }
            case Key::S:
            {
                if (control && shift)
                {
                    HIMII_CORE_INFO("场景另存为");
                }
                else if (control)
                {
                    HIMII_CORE_INFO("保存场景");
                }
                break;
            }
        }
        return false;
    }

    bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
    {
        return false;
    }
}
