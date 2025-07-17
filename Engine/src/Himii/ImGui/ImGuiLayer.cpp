#include "ImGuiLayer.h"
#include "imgui_internal.h"
#include "Himii/Core/Application.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "Platform/OpenGL/imgui_impl_opengl3.h"

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
        /* 使用深色主题 */
        ImGui::StyleColorsClassic();

        ImGuiIO &io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable ImGui mouse cursors
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // Enable SetMousePos backend function

        ImGui_ImplOpenGL3_Init("#version 410");
    }

   void ImGuiLayer::OnDetach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::DestroyContext();
    }

   void ImGuiLayer::OnUpdate()
    {
       ImGuiIO &io = ImGui::GetIO();
        Application &app = Application::Get();
       io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

       float time = (float)glfwGetTime();
       io.DeltaTime = time - m_Time;
       m_Time = time;

       ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();

        static bool show = true;
        ImGui::ShowDemoWindow(&show);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

   void ImGuiLayer::OnEvent(Event &event)
    {
       
    }
}