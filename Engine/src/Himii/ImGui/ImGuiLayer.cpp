#include "ImGuiLayer.h"
#include "imgui_internal.h"
#include "Himii/Core/Application.h"
#include "Himii/Core/Core.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "Platform/OpenGL/imgui_impl_opengl3.h"
#include "Platform/OpenGL/imgui_impl_glfw.h"

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
        io.ConfigFlags |= ImGuiDebugLogFlags_EventDocking;    // Enable Docking debug logging
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // Enable ImGui mouse cursors
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // Enable SetMousePos backend function

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

   void ImGuiLayer::OnDetach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
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

        ImGui::ShowDemoWindow();

        ImGui::Begin("Demo Window");
        ImGui::Text("Hello, world!");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

   void ImGuiLayer::OnEvent(Event &event)
    {
       EventDispatcher dispatcher(event);
       dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressed));
       dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleased));
       dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseMoved));
       dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiLayer::OnMouseScrolled));
       dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyPressed));
       dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(ImGuiLayer::OnKeyReleased));
       // dispatcher.Dispatch<KeyTypedEvent>(HIMII_BIND_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
       dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(ImGuiLayer::OnWindowResizeEvent));
    }


   bool ImGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[event.GetMouseButton()] = true;
        return false;
    }

   bool ImGuiLayer::OnMouseButtonReleased(MouseButtonReleasedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[event.GetMouseButton()] = false;
        return false;
    }

   bool ImGuiLayer::OnMouseMoved(MouseMovedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = ImVec2(event.GetX(), event.GetY());
        return false;
    }

   bool ImGuiLayer::OnMouseScrolled(MouseScrolledEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddMouseWheelEvent(event.GetXOffset(), event.GetYOffset());
        return false;
    }

   bool ImGuiLayer::OnKeyPressed(KeyPressedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddKeyEvent(ImGui_ImplGlfw_KeyToImGuiKey(event.GetKeyCode()), true);
        io.SetKeyEventNativeData(ImGui_ImplGlfw_KeyToImGuiKey(event.GetKeyCode()), event.GetKeyCode(), 0);
        return false;
    }

   bool ImGuiLayer::OnKeyReleased(KeyReleasedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddKeyEvent(ImGui_ImplGlfw_KeyToImGuiKey(event.GetKeyCode()), false);
        io.SetKeyEventNativeData(ImGui_ImplGlfw_KeyToImGuiKey(event.GetKeyCode()), event.GetKeyCode(), 0);
        return false;
    }

   /*void ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddInputCharacter(event.GetKeyCode());
    }*/

   bool ImGuiLayer::OnWindowResizeEvent(WindowResizeEvent &event)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(event.GetWidth(), event.GetHeight());
        io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
        return false;
    }
}