#include "ImGuiLayer.h"
#include "GLFW/glfw3.h"
#include "Himii/Core/Application.h"
#include "Himii/Core/FileSystem.h"
#include "imgui_internal.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGuizmo.h"

namespace Himii
{
    namespace
    {
        static bool IniHasEditorDockLayout(const std::filesystem::path &layout_ini_path)
        {
            std::ifstream input(layout_ini_path);
            if (!input.is_open())
                return false;

            std::stringstream buffer;
            buffer << input.rdbuf();
            const std::string content = buffer.str();

            return content.find("[Docking][Data]") != std::string::npos &&
                   content.find("[Window][ViewPort]") != std::string::npos &&
                   content.find("[Window][Scene Hierarchy]") != std::string::npos;
        }

        void PrepareEditorLayoutIniOnStartup(const std::filesystem::path &layout_ini_path)
        {
            if (!std::filesystem::exists(layout_ini_path))
                return;

            if (IniHasEditorDockLayout(layout_ini_path))
                return;

            std::error_code error_code;
            std::filesystem::remove(layout_ini_path, error_code);
        }
    }

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
    {
    }

    void ImGuiLayer::OnAttach()
    {
        HIMII_PROFILE_FUNCTION();

        HIMII_CORE_INFO("ImGuiLayer::OnAttach()");

        ImGui::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        Application &app = Application::Get();
        GLFWwindow *window = static_cast<GLFWwindow *>(app.GetWindow().GetNativeWindow());

        (void)io;
        ImGui::StyleColorsDark();

        m_IniFilePath = (Application::Get().GetExecutableDir() / "editor_layout.ini").string();
        HIMII_CORE_INFO("Editor layout ini: {0}", m_IniFilePath);

        io.IniFilename = nullptr;

        // 不启用 Keyboard Nav：Space/方向键会进游戏脚本（Editor Play 与 Runtime 一致）。
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        ApplyEditorTheme();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");

        LoadEditorFonts();
    }

    void ImGuiLayer::EnableLayoutPersistence()
    {
        if (m_LayoutPersistenceEnabled)
            return;

        PrepareEditorLayoutIniOnStartup(m_IniFilePath);

        // Splash/Hub 阶段 IniFilename 为 null 时，ImGui 已在首帧把 SettingsLoaded 置 true 且不会自动再读盘。
        // 进入编辑器前必须清空内存中的 ini 状态并显式加载 editor_layout.ini。
        ImGuiContext &imgui_context = *ImGui::GetCurrentContext();
        ImGui::ClearIniSettings();
        imgui_context.SettingsLoaded = false;

        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = m_IniFilePath.c_str();

        if (std::filesystem::exists(m_IniFilePath))
        {
            ImGui::LoadIniSettingsFromDisk(m_IniFilePath.c_str());
            HIMII_CORE_INFO("Loaded editor layout from: {0}", m_IniFilePath);
        }
        else
        {
            imgui_context.SettingsLoaded = true;
            HIMII_CORE_INFO("No editor layout ini found, default dock layout will be built: {0}", m_IniFilePath);
        }

        m_LayoutPersistenceEnabled = true;
    }

    void ImGuiLayer::LoadEditorFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        Application &application = Application::Get();
        GLFWwindow *window = static_cast<GLFWwindow *>(application.GetWindow().GetNativeWindow());

        float x_scale = 1.0f;
        float y_scale = 1.0f;
        glfwGetWindowContentScale(window, &x_scale, &y_scale);
        const float user_interface_scale = x_scale;
        HIMII_CORE_INFO("Window Content Scale: {0}, {1}", x_scale, y_scale);

        io.Fonts->Clear();
        const std::filesystem::path fontPath = FileSystem::MaterializeLooseFile("assets/fonts/msyh.ttc");
        const float font_size = 15.0f * user_interface_scale;
        io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), font_size);
        io.FontDefault = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), font_size, nullptr,
                                                       io.Fonts->GetGlyphRangesChineseFull());
        io.Fonts->Build();

        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(user_interface_scale);

        ImGui_ImplOpenGL3_DestroyDeviceObjects();
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    }

    void ImGuiLayer::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();
        HIMII_CORE_INFO("ImGuiLayer::OnDetach()");

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
        if (m_BlockEvents)
        {
            ImGuiIO &io = ImGui::GetIO();
            e.Handled |= e.IsInCategroy(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCategroy(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }
    }

    void ImGuiLayer::Begin()
    {
        HIMII_PROFILE_FUNCTION();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
    }

    void ImGuiLayer::End()
    {
        HIMII_PROFILE_FUNCTION();

        ImGuiIO &io = ImGui::GetIO();
        Application &app = Application::Get();
        io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void ImGuiLayer::ApplyEditorTheme()
    {
        ImGuiStyle &style = ImGui::GetStyle();
        ImVec4 *colors = style.Colors;

        const ImVec4 accent_blue = ImVec4{0.0f, 0.47f, 0.84f, 1.0f};
        const ImVec4 accent_blue_dim = ImVec4{0.0f, 0.35f, 0.65f, 1.0f};

        // 面板 / Project Hub 等内容区背景
        const ImVec4 panel_content_background = ImVec4{0.23f, 0.23f, 0.23f, 1.0f};

        // 输入框：比面板背景更暗（凹槽）
        const ImVec4 input_field = ImVec4{0.16f, 0.16f, 0.16f, 1.0f};
        const ImVec4 input_field_hover = ImVec4{0.19f, 0.19f, 0.19f, 1.0f};
        const ImVec4 input_field_active = ImVec4{0.18f, 0.18f, 0.18f, 1.0f};

        // 组件折叠标题条（比面板更暗）
        const ImVec4 inspector_section_header = ImVec4{0.16f, 0.16f, 0.16f, 1.0f};

        // 标题栏条背景（整条 Tab 条保持深色）
        const ImVec4 recessed_title = ImVec4{0.12f, 0.12f, 0.12f, 1.0f};

        // 单个 Tab：未选中略亮；选中与面板背景一致
        const ImVec4 tab_unselected = ImVec4{0.18f, 0.18f, 0.18f, 1.0f};
        const ImVec4 tab_hover = ImVec4{0.20f, 0.20f, 0.20f, 1.0f};
        const ImVec4 tab_selected = panel_content_background;

        const ImVec4 shell_mid = ImVec4{0.28f, 0.28f, 0.28f, 1.0f};
        const ImVec4 shell_hover = ImVec4{0.32f, 0.32f, 0.32f, 1.0f};
        const ImVec4 shell_button = ImVec4{0.28f, 0.28f, 0.28f, 1.0f};
        const ImVec4 shell_button_active = ImVec4{0.26f, 0.26f, 0.26f, 1.0f};

        const ImVec4 header_active = ImVec4{0.0f, 0.40f, 0.72f, 0.55f};

        const ImVec4 border_subtle = ImVec4{0.26f, 0.26f, 0.26f, 1.0f};
        const ImVec4 text_primary = ImVec4{0.90f, 0.90f, 0.90f, 1.0f};
        const ImVec4 text_disabled = ImVec4{0.50f, 0.50f, 0.50f, 1.0f};

        style.WindowRounding = 2.0f;
        style.ChildRounding = 2.0f;
        style.FrameRounding = 2.0f;
        style.PopupRounding = 2.0f;
        style.ScrollbarRounding = 4.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.TabBarOverlineSize = 0.0f;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.WindowPadding = ImVec2{8.0f, 8.0f};
        style.FramePadding = ImVec2{6.0f, 4.0f};
        style.ItemSpacing = ImVec2{8.0f, 6.0f};

        colors[ImGuiCol_Text] = text_primary;
        colors[ImGuiCol_TextDisabled] = text_disabled;

        colors[ImGuiCol_WindowBg] = panel_content_background;
        colors[ImGuiCol_ChildBg] = panel_content_background;
        colors[ImGuiCol_PopupBg] = ImVec4{shell_mid.x, shell_mid.y, shell_mid.z, 0.98f};

        colors[ImGuiCol_Border] = border_subtle;
        colors[ImGuiCol_BorderShadow] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};

        // 输入框 / 下拉 / 引用框
        colors[ImGuiCol_FrameBg] = input_field;
        colors[ImGuiCol_FrameBgHovered] = input_field_hover;
        colors[ImGuiCol_FrameBgActive] = input_field_active;

        colors[ImGuiCol_TitleBg] = recessed_title;
        colors[ImGuiCol_TitleBgActive] = recessed_title;
        colors[ImGuiCol_TitleBgCollapsed] = recessed_title;

        colors[ImGuiCol_MenuBarBg] = recessed_title;

        colors[ImGuiCol_ScrollbarBg] = panel_content_background;
        colors[ImGuiCol_ScrollbarGrab] = shell_mid;
        colors[ImGuiCol_ScrollbarGrabHovered] = shell_hover;
        colors[ImGuiCol_ScrollbarGrabActive] = accent_blue_dim;

        colors[ImGuiCol_CheckMark] = accent_blue;
        colors[ImGuiCol_SliderGrab] = accent_blue_dim;
        colors[ImGuiCol_SliderGrabActive] = accent_blue;

        colors[ImGuiCol_Button] = shell_button;
        colors[ImGuiCol_ButtonHovered] = shell_hover;
        colors[ImGuiCol_ButtonActive] = shell_button_active;

        colors[ImGuiCol_Header] = inspector_section_header;
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.0f, 0.35f, 0.65f, 0.45f};
        colors[ImGuiCol_HeaderActive] = header_active;

        colors[ImGuiCol_Separator] = border_subtle;
        colors[ImGuiCol_SeparatorHovered] = accent_blue_dim;
        colors[ImGuiCol_SeparatorActive] = accent_blue;

        colors[ImGuiCol_ResizeGrip] = ImVec4{0.23f, 0.23f, 0.23f, 0.30f};
        colors[ImGuiCol_ResizeGripHovered] = accent_blue_dim;
        colors[ImGuiCol_ResizeGripActive] = accent_blue;

        colors[ImGuiCol_Tab] = tab_unselected;
        colors[ImGuiCol_TabHovered] = tab_hover;
        colors[ImGuiCol_TabSelected] = tab_selected;
        colors[ImGuiCol_TabSelectedOverline] = tab_selected;
        colors[ImGuiCol_TabDimmed] = tab_unselected;
        colors[ImGuiCol_TabDimmedSelected] = tab_selected;
        colors[ImGuiCol_TabDimmedSelectedOverline] = tab_selected;

        colors[ImGuiCol_DockingPreview] = ImVec4{0.0f, 0.47f, 0.84f, 0.35f};
        colors[ImGuiCol_DockingEmptyBg] = panel_content_background;

        colors[ImGuiCol_TextSelectedBg] = ImVec4{0.0f, 0.47f, 0.84f, 0.35f};

        colors[ImGuiCol_NavHighlight] = accent_blue;
        colors[ImGuiCol_NavWindowingHighlight] = accent_blue;
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const
    {
        return GImGui->ActiveId;
    }

    void ImGuiLayer::OnImGuiRender()
    {
    }
}
