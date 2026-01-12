#include "ImGuiLayer.h"
#include "GLFW/glfw3.h"
#include "Himii/Core/Application.h"
#include "imgui_internal.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ImGuizmo.h"

namespace Himii
{
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
        /* 使用深色主题 */
        ImGui::StyleColorsDark();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

        const char *font_path = "assets/fonts/msyh.ttc"; // 请替换为你的字体实际路径和文件名
        float font_size = 15.0f;

        io.Fonts->AddFontFromFileTTF(font_path, font_size);
        io.FontDefault =
                io.Fonts->AddFontFromFileTTF(font_path, font_size, nullptr, io.Fonts->GetGlyphRangesChineseFull());

        ImGuiStyle &style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        SetDarkThemeColors();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void ImGuiLayer::OnDetach()
    {
        HIMII_PROFILE_FUNCTION();

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

        // Rendering
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

    void ImGuiLayer::SetDarkThemeColors()
    {
        auto &style = ImGui::GetStyle();
        auto &colors = style.Colors;

        style.WindowRounding = 5.0f;  // Subtle rounding
        style.FrameRounding = 4.0f;   // Boxy but soft
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 10.0f;
        style.GrabRounding = 8.0f;
        style.TabRounding = 4.0f;     // Tabs look better with more rounding on top

        style.WindowBorderSize = 0.0f; // Subtle border
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;
        
        // Backgrounds (Softer Dark Grey)
        colors[ImGuiCol_WindowBg] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};
        colors[ImGuiCol_PopupBg] = ImVec4{0.14f, 0.14f, 0.14f, 0.94f};
        
        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.26f, 0.26f, 0.26f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.34f, 0.34f, 0.34f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.30f, 0.30f, 0.30f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.26f, 0.26f, 0.26f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.30f, 0.30f, 0.30f, 1.0f};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.18f, 0.18f, 0.18f, 1.0f};

        // Frame BG (Input fields - Darker than window for contrast)
        colors[ImGuiCol_FrameBg] = ImVec4{0.14f, 0.14f, 0.14f, 1.0f}; 
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.16f, 0.16f, 0.16f, 1.0f};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.14f, 0.14f, 0.14f, 1.0f};

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f}; // Match WindowBg
        colors[ImGuiCol_TabHovered] = ImVec4{0.30f, 0.30f, 0.32f, 1.0f}; 
        colors[ImGuiCol_TabActive] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};  
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{0.11f, 0.11f, 0.11f, 1.0f}; 
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.11f, 0.11f, 0.11f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.11f, 0.11f, 0.11f, 1.0f};
        
        // Separators
        colors[ImGuiCol_Separator] = ImVec4{0.23f, 0.23f, 0.23f, 1.0f}; 
        colors[ImGuiCol_SeparatorHovered] = ImVec4{0.32f, 0.32f, 0.32f, 1.0f};
        colors[ImGuiCol_SeparatorActive] = ImVec4{0.28f, 0.28f, 0.28f, 1.0f};
        
        // Resize Grip
        colors[ImGuiCol_ResizeGrip] = ImVec4{0.30f, 0.30f, 0.30f, 0.25f};
        
        // Standard Text
        colors[ImGuiCol_Text] = ImVec4{0.85f, 0.85f, 0.85f, 1.0f}; // Soft white
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const
    {
        return GImGui->ActiveId;
    }

    void ImGuiLayer::OnImGuiRender()
    {
    }
} // namespace Himii
