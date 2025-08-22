#include "EditorLayer.h"
using namespace Himii;
// 需要组件定义
#include "Himii/Scene/Components.h"
// for snprintf
#include <cstdio>

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
    ImGui::Begin("Game");
    ImVec2 gAvail = ImGui::GetContentRegionAvail();
    m_GameHovered = ImGui::IsWindowHovered();
    m_GameFocused = ImGui::IsWindowFocused();
    if (m_GameTexture)
    {
        ImGui::Image((ImTextureID)(intptr_t)m_GameTexture, gAvail, ImVec2(0,1), ImVec2(1,0));
        if (ImGui::BeginPopupContextWindow())
        {
            ImGui::Text("Size: %.0f x %.0f", gAvail.x, gAvail.y);
            ImGui::EndPopup();
        }
    }
    else
    {
        ImGui::Dummy(gAvail);
    }
    if (gAvail.x != m_LastGameAvail.x || gAvail.y != m_LastGameAvail.y)
    {
        m_LastGameAvail = gAvail;
    }
    ImGui::End();

    // 根据 Scene/Game 的悬停状态，决定是否让 ImGui 层阻断底层输入
    // 目标：当鼠标在 Scene 上滚动时，让滚轮事件传到底层相机控制器
    auto& app = Himii::Application::Get();
    if (auto* imgui = app.GetImGuiLayer())
    {
        const bool wantBlock = !(m_SceneHovered || m_SceneFocused || m_GameHovered || m_GameFocused);
        imgui->BlockEvents(wantBlock);
    }

    // Hierarchy 面板：列出场景中的实体（简单策略：有 Transform 的都显示）
    ImGui::Begin("Hierarchy");
    if (m_ActiveScene)
    {
        auto &reg = m_ActiveScene->Registry();
        // 遍历所有带 Transform 的实体
        auto view = reg.view<Transform>();
        for (auto e : view)
        {
            // 名称优先：Tag.name，否则显示生成的 ID 或句柄值
            std::string label;
        if (auto* t = reg.try_get<Tag>(e))
                label = t->name.empty() ? std::string("Entity") : t->name;
            else
            {
                if (auto* id = reg.try_get<ID>(e)) {
                    char tmp[64]; std::snprintf(tmp, sizeof(tmp), "Entity %llu", (unsigned long long)(uint64_t)id->id);
                    label = tmp;
                } else {
            char tmp[64]; std::snprintf(tmp, sizeof(tmp), "Entity %u", (unsigned)e);
                    label = tmp;
                }
            }
            // 渲染选择项
            bool selected = (e == m_SelectedEntity);
            if (ImGui::Selectable(label.c_str(), selected))
                m_SelectedEntity = e;
        }
        if (ImGui::Button("Create Entity"))
        {
            auto e = m_ActiveScene->CreateEntity("Entity");
            // 默认会有 Transform，这里附加一个默认颜色的 SpriteRenderer 便于可见
            reg.emplace_or_replace<SpriteRenderer>(e, glm::vec4{0.8f, 0.8f, 0.8f, 1.0f});
            m_SelectedEntity = e;
        }
        if (m_SelectedEntity != entt::null && !reg.valid(m_SelectedEntity))
            m_SelectedEntity = entt::null;
    }
    else
    {
        ImGui::TextUnformatted("No active Scene.");
    }
    ImGui::End();

    // Inspector 面板：显示并编辑选中实体的常用组件
    ImGui::Begin("Inspector");
    if (m_ActiveScene && m_SelectedEntity != entt::null && m_ActiveScene->Registry().valid(m_SelectedEntity))
    {
        auto &reg = m_ActiveScene->Registry();
        if (reg.any_of<Tag>(m_SelectedEntity))
        {
            auto &tag = reg.get<Tag>(m_SelectedEntity);
            char nameBuf[128];
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", tag.name.c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
                tag.name = nameBuf;
        }
        if (reg.any_of<Transform>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &tr = reg.get<Transform>(m_SelectedEntity);
                ImGui::DragFloat3("Position", &tr.Position.x, 0.01f);
                ImGui::DragFloat3("Rotation", &tr.Rotation.x, 0.5f);
                ImGui::DragFloat3("Scale", &tr.Scale.x, 0.01f, 0.01f, 100.0f);
            }
        }
        if (reg.any_of<SpriteRenderer>(m_SelectedEntity))
        {
            if (ImGui::CollapsingHeader("SpriteRenderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto &sr = reg.get<SpriteRenderer>(m_SelectedEntity);
                ImGui::ColorEdit4("Color", &sr.color.x);
                ImGui::DragFloat("Tiling", &sr.tiling, 0.01f, 0.1f, 100.0f);
            }
        }
        if (ImGui::Button("Delete Entity"))
        {
            m_ActiveScene->DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = entt::null;
        }
    }
    else
    {
        ImGui::TextUnformatted("Select an entity in Hierarchy.");
    }
    ImGui::End();

    ImGui::Begin("Console");
    ImGui::TextUnformatted("Logs appear here.");
    ImGui::End();

    ImGui::End();
}

// 可选：供外部查询 Scene 面板的期望尺寸
// 我们用 width/height 字段让运行层设置与读取
