#include "Example2D.h"
#include "EditorLayer.h"
#include "Move2DScript.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Example2D::Example2D() :
    Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
{
}

void Example2D::OnAttach()
{
    HIMII_PROFILE_FUNCTION();

    // 创建离屏帧缓冲，尺寸先用窗口大小，后续由 EditorLayer 面板驱动调整
    auto &app = Himii::Application::Get();
    Himii::FramebufferSpecification fbSpec{app.GetWindow().GetWidth(), app.GetWindow().GetHeight()};
    fbSpec.Attachments = {Himii::FramebufferFormat::RGBA8, Himii::FramebufferFormat::RED_INTEGER,
                          Himii::FramebufferFormat::Depth};
    m_SceneFramebuffer = Himii::Framebuffer::Create(fbSpec);

    // 最小 ECS 场景：创建几个彩色方块实体（使用 Entity 包装）
    {
        auto e1 = m_Scene.CreateEntity("Red Quad");
        auto &tr1 = e1.GetComponent<Himii::Transform>();
        tr1.Position = {-0.5f, -0.5f, 0.0f};
        e1.AddComponent<Himii::SpriteRenderer>(glm::vec4{0.9f, 0.2f, 0.3f, 1.0f});

        auto e2 = m_Scene.CreateEntity("Green Quad");
        auto &tr2 = e2.GetComponent<Himii::Transform>();
        tr2.Position = {0.3f, 0.1f, 0.0f};
        e2.AddComponent<Himii::SpriteRenderer>(glm::vec4{0.2f, 0.8f, 0.4f, 1.0f});

        auto e3 = m_Scene.CreateEntity("Blue Quad");
        auto &tr3 = e3.GetComponent<Himii::Transform>();
        tr3.Position = {-0.2f, 0.6f, 0.0f};
        e3.AddComponent<Himii::SpriteRenderer>(glm::vec4{0.3f, 0.5f, 1.0f, 1.0f});

        auto e4 = m_Scene.CreateEntity("My Quad");
        // 默认构造 SpriteRenderer（白色），或传入颜色
        e4.AddComponent<Himii::SpriteRenderer>(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
        // 绑定二维移动脚本
        {
            auto &nsc = e4.AddComponent<Himii::NativeScriptComponent>();
            nsc.Bind<Move2DScript>();
        }
    }
}
void Example2D::OnDetach()
{
    HIMII_PROFILE_FUNCTION();
}

void Example2D::OnUpdate(Himii::Timestep ts)
{
    HIMII_PROFILE_FUNCTION();

    {
        HIMII_PROFILE_SCOPE("CameraController::OnUpdate");
        m_CameraController.OnUpdate(ts);
    }
    m_SceneFramebuffer->Resize(1280, 720); // 临时写死，后续由 EditorLayer 面板驱动调整

    // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
    Himii::Renderer2D::ResetStats();

    m_SceneFramebuffer->Bind();
    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    m_SceneFramebuffer->ClearAttachment(1, -1); // 1号附件清除为 -1（无实体）

    HIMII_PROFILE_SCOPE("Renderer Draw");
    Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());
    m_Scene.OnUpdate(ts);
    Himii::Renderer2D::EndScene();
    m_SceneFramebuffer->Unbind();
}
void Example2D::OnImGuiRender()
{
    HIMII_PROFILE_FUNCTION();

    static bool dockingEnable = true;

    if (dockingEnable)
    {
        static bool dockspaceOpen = true;
        static bool  opt_fullscreen = true;
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

        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO &io = ImGui::GetIO();

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

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

        ImGui::Begin("Setting");
        auto stats = Himii::Renderer2D::GetStatistics();
        ImGui::Text("Renderer2D Stats");
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quad Count: %d", stats.QuadCount);
        ImGui::Text("Total Vertex Count: %d", stats.GetTotalVertexCount());
        ImGui::Text("Total Index Count: %d", stats.GetTotalIndexCount());

        uint32_t textureID = m_SceneFramebuffer->GetColorAttachmentRendererID();
        ImGui::Image((void *)textureID, ImVec2{640, 360});
        ImGui::End();
    }
}

void Example2D::OnEvent(Himii::Event &event)
{
    // 仅当 Scene 视口被悬停或聚焦时才处理滚轮/键盘等事件
    EditorLayer *editorRef = nullptr;
    for (auto *layer: Himii::Application::Get().GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer)))
            break;
    if (!editorRef || editorRef->IsSceneHovered() || editorRef->IsSceneFocused())
        m_CameraController.OnEvent(event);
}
