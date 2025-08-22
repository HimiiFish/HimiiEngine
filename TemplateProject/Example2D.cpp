#include "Example2D.h"
#include "imgui.h"
#include "EditorLayer.h"

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
    m_SceneFramebuffer = Himii::Framebuffer::Create(fbSpec);

    // 最小 ECS 场景：创建几个彩色方块实体
    {
        auto e1 = m_Scene.CreateEntity("Red Quad");
    auto &tr1 = m_Scene.Registry().get<Himii::Transform>(e1);
    tr1.Position = {-0.5f, -0.5f, 0.0f};
        m_Scene.Registry().emplace<Himii::SpriteRenderer>(e1, glm::vec4{0.9f, 0.2f, 0.3f, 1.0f});

        auto e2 = m_Scene.CreateEntity("Green Quad");
    auto &tr2 = m_Scene.Registry().get<Himii::Transform>(e2);
    tr2.Position = {0.3f, 0.1f, 0.0f};
        m_Scene.Registry().emplace<Himii::SpriteRenderer>(e2, glm::vec4{0.2f, 0.8f, 0.4f, 1.0f});

        auto e3 = m_Scene.CreateEntity("Blue Quad");
    auto &tr3 = m_Scene.Registry().get<Himii::Transform>(e3);
    tr3.Position = {-0.2f, 0.6f, 0.0f};
        m_Scene.Registry().emplace<Himii::SpriteRenderer>(e3, glm::vec4{0.3f, 0.5f, 1.0f, 1.0f});
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

    // 从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
    Himii::Renderer2D::ResetStats();
    EditorLayer *editorRef = nullptr;
    for (auto *layer : Himii::Application::Get().GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer))) break;
    bool resized = false;
    if (editorRef && m_SceneFramebuffer)
    {
        ImVec2 desired = editorRef->GetSceneDesiredSize();
        uint32_t newW = (uint32_t)std::max(1.0f, desired.x);
        uint32_t newH = (uint32_t)std::max(1.0f, desired.y);
        auto &spec = m_SceneFramebuffer->GetSpecification();
        if (newW != spec.Width || newH != spec.Height)
        {
            m_SceneFramebuffer->Resize(newW, newH);
            // 同步相机投影，保持 1:1 比例不随视口拉伸而变形
            m_CameraController.OnResize((float)newW, (float)newH);
            resized = true;
        }
    }
    // 渲染到离屏 FBO
    if (m_SceneFramebuffer)
    {
        m_SceneFramebuffer->Bind();
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();
    }
    
    {
        HIMII_PROFILE_SCOPE("Renderer Draw");
        Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());
        m_Scene.OnUpdate(ts);
        Himii::Renderer2D::EndScene();
    }

    // 解绑 FBO 并把纹理与尺寸交给 EditorLayer 展示
    if (m_SceneFramebuffer)
    {
        m_SceneFramebuffer->Unbind();
        if (editorRef)
        {
            editorRef->SetSceneTexture(m_SceneFramebuffer->GetColorAttachmentRendererID());
            editorRef->SetSceneSize(m_SceneFramebuffer->GetSpecification().Width,
                                    m_SceneFramebuffer->GetSpecification().Height);
            // 将当前 Scene 指给编辑器，供 Hierarchy/Inspector 使用
            if (editorRef->GetActiveScene() != &m_Scene)
                editorRef->SetActiveScene(&m_Scene);
        }
    }
}
void Example2D::OnImGuiRender()
{
    HIMII_PROFILE_FUNCTION();
    ImGui::Begin("ECS 示例");
    ImGui::Text("场景包含 3 个 SpriteRenderer 实体");
    ImGui::Text("Renderer2D Stats");
    ImGui::Text("Draw Calls: %d", Himii::Renderer2D::GetStatistics().DrawCalls);
    ImGui::Text("Quad Count: %d", Himii::Renderer2D::GetStatistics().QuadCount);
    ImGui::Text("Total Vertex Count: %d", Himii::Renderer2D::GetStatistics().GetTotalVertexCount());
    ImGui::Text("Total Index Count: %d", Himii::Renderer2D::GetStatistics().GetTotalIndexCount());
    ImGui::End();
}

void Example2D::OnEvent(Himii::Event &event)
{
    // 仅当 Scene 视口被悬停或聚焦时才处理滚轮/键盘等事件
    EditorLayer *editorRef = nullptr;
    for (auto *layer : Himii::Application::Get().GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer))) break;
    if (!editorRef || editorRef->IsSceneHovered() || editorRef->IsSceneFocused())
        m_CameraController.OnEvent(event);
}
