#include "EditorLayer.h"
using namespace Himii;

void EditorLayer::OnImGuiRender()
{
    static bool dockspaceOpen = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Himii Editor", &dockspaceOpen, window_flags);

    if (!opt_padding)
        ImGui::PopStyleVar();
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Dockspace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("HIMII_DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
    }

    // Menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open...")) {}
            if (ImGui::MenuItem("Save")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) { Himii::Application::Get().Close(); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            ImGui::MenuItem("Inspector", nullptr, true);
            ImGui::MenuItem("Hierarchy", nullptr, true);
            ImGui::MenuItem("Console", nullptr, true);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Scene 视口（显示 FBO 颜色纹理）
    ImGui::Begin("Scene");
    ImVec2 avail = ImGui::GetContentRegionAvail();
    m_SceneHovered = ImGui::IsWindowHovered();
    m_SceneFocused = ImGui::IsWindowFocused();
    if (m_SceneTexture)
    {
        // 注意：OpenGL 的纹理 Y 轴需翻转，ImGui::Image 使用 UV 坐标可倒置
        ImGui::Image((ImTextureID)(intptr_t)m_SceneTexture, avail, ImVec2(0,1), ImVec2(1,0));
        if (ImGui::BeginPopupContextWindow())
        {
            ImGui::Text("Size: %.0f x %.0f", avail.x, avail.y);
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::Dummy(avail);
    }
    // 记录尺寸以便外部决定是否 Resize FBO
    if (avail.x != m_LastSceneAvail.x || avail.y != m_LastSceneAvail.y)
    {
        m_LastSceneAvail = avail;
        // 外部可在下一帧查询 GetSceneDesiredSize 来调整
    }
    ImGui::End();

    // Game 视口

    // Placeholder panels
    ImGui::Begin("Hierarchy");
    ImGui::TextUnformatted("[Scene] Terrain");
    ImGui::TextUnformatted("[Camera] EditorCamera");
    ImGui::End();

    ImGui::Begin("Inspector");
    ImGui::TextUnformatted("Select an item to see details.");
    ImGui::End();

    ImGui::Begin("Console");
    ImGui::TextUnformatted("Logs appear here.");
    ImGui::End();

    ImGui::End();
}
