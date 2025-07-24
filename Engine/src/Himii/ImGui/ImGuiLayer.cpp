#include "ImGuiLayer.h"
#include "GLFW/glfw3.h"
#include "Himii/Core/Application.h"
#include "Himii/Core/Core.h"
#include "imgui_internal.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace Himii
{
    static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int key)
    {
        switch (key)
        {
            case GLFW_KEY_TAB:
                return ImGuiKey_Tab;
            case GLFW_KEY_LEFT:
                return ImGuiKey_LeftArrow;
            case GLFW_KEY_RIGHT:
                return ImGuiKey_RightArrow;
            case GLFW_KEY_UP:
                return ImGuiKey_UpArrow;
            case GLFW_KEY_DOWN:
                return ImGuiKey_DownArrow;
            case GLFW_KEY_PAGE_UP:
                return ImGuiKey_PageUp;
            case GLFW_KEY_PAGE_DOWN:
                return ImGuiKey_PageDown;
            case GLFW_KEY_HOME:
                return ImGuiKey_Home;
            case GLFW_KEY_END:
                return ImGuiKey_End;
            case GLFW_KEY_INSERT:
                return ImGuiKey_Insert;
            case GLFW_KEY_DELETE:
                return ImGuiKey_Delete;
            case GLFW_KEY_BACKSPACE:
                return ImGuiKey_Backspace;
            case GLFW_KEY_SPACE:
                return ImGuiKey_Space;
            case GLFW_KEY_ENTER:
                return ImGuiKey_Enter;
            case GLFW_KEY_ESCAPE:
                return ImGuiKey_Escape;
            default:
                if (key >= '0' && key <= '9')
                    return (ImGuiKey)(ImGuiKey_0 + (key - '0'));
                if (key >= 'A' && key <= 'Z')
                    return (ImGuiKey)(ImGuiKey_A + (key - 'A'));
                if (key >= 'a' && key <= 'z')
                    return (ImGuiKey)(ImGuiKey_A + (key - 'a'));
                break;
        }
        return ImGuiKey_None; // 未知键
    }

    ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
    {
    }

    void ImGuiLayer::OnAttach()
    {
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
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

        const char *font_path = "assets/msyh.ttc"; // 请替换为你的字体实际路径和文件名
        float font_size = 14.0f;

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
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::End()
    {
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
        auto &colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Headers
        colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Buttons
        colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Frame BG
        colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    }

    uint32_t ImGuiLayer::GetActiveWidgetID() const
    {
        return GImGui->ActiveId;
    }

    void ImGuiLayer::OnImGuiRender()
    {
        ImGui::ShowDemoWindow();
    }
} // namespace Himii
