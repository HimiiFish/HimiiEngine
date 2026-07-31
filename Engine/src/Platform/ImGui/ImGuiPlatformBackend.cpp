#include "Hepch.h"
#include "Platform/ImGui/ImGuiPlatformBackend.h"

#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace Himii
{
    void ImGuiPlatformBackend::Initialize(void *nativeWindow)
    {
        GLFWwindow *window = static_cast<GLFWwindow *>(nativeWindow);
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void ImGuiPlatformBackend::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void ImGuiPlatformBackend::NewFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
    }

    void ImGuiPlatformBackend::RenderDrawData(ImDrawData *drawData)
    {
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }

    void ImGuiPlatformBackend::RenderPlatformWindows()
    {
        GLFWwindow *backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupCurrentContext);
    }

    void ImGuiPlatformBackend::RecreateDeviceObjects()
    {
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    }
}
