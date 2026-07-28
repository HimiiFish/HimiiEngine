#include "Hepch.h"
#include "Himii/Core/Application.h"
#include "Himii/Core/FileSystem.h"
#include "Himii/Renderer/Renderer.h"
#include "Himii/Core/Input.h"
#include <GLFW/glfw3.h>
#ifndef HIMII_PLATFORM_WINDOWS
#include <unistd.h>
#endif

#ifdef HIMII_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include "Himii/Scripting/ScriptEngine.h"
#include "Himii/Scripting/ScriptCompiler.h"
#include "Himii/Core/JobSystem.h"
#include "Himii/Audio/AudioEngine.h"

namespace Himii
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const std::string &name, ApplicationCommandLineArgs args,
                             const WindowProps &window_props)
        : m_CommandLineArgs(args)
    {
        HIMII_PROFILE_FUNCTION();


        SetEnvironmentVariables();
        s_Instance = this;
        WindowProps initial_props = window_props;
        if (initial_props.Title.empty())
            initial_props.Title = name;
        m_Window = Window::Create(initial_props);
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        Renderer::Init();
        Renderer::OnWindowResize(m_Window->GetFramebufferWidth(), m_Window->GetFramebufferHeight());
        AudioEngine::Init();
        JobSystem::Initialize();
        ScriptEngine::Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        HIMII_PROFILE_FUNCTION();

        ScriptCompiler::Shutdown();
        ScriptEngine::Shutdown();
        JobSystem::Shutdown();
        AudioEngine::Shutdown();
        FileSystem::Shutdown();
        s_Instance = nullptr;
    }

    void Application::PushLayer(Layer *layer)
    {
        HIMII_PROFILE_FUNCTION();

        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *overlay)
    {
        HIMII_PROFILE_FUNCTION();

        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event &e)
    {
        HIMII_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClosed));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend();++it)
        {
            if (e.Handled)
                break;
            (*it)->OnEvent(e);
        }
    }

    void Application::SetEnvironmentVariables()
    {
#ifdef HIMII_PLATFORM_WINDOWS
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::filesystem::path executablePath = std::filesystem::path(buffer).parent_path();
#else
        char buffer[1024];
        ssize_t count = readlink("/proc/self/exe", buffer, 1024);
        std::filesystem::path executablePath;
        if (count != -1)
            executablePath = std::filesystem::path(std::string(buffer, (count > 0) ? count : 0)).parent_path();
        else
            executablePath = std::filesystem::current_path();
#endif
        m_ExecutableDir = executablePath;

        // 打包游戏常被从其它工作目录启动；强制 cwd=exe 目录，确保能找到 Game.hproj / assets。
        {
            std::error_code currentPathError;
            std::filesystem::current_path(m_ExecutableDir, currentPathError);
            if (currentPathError)
            {
                HIMII_CORE_WARNING("Failed to set working directory to executable dir '{0}': {1}",
                                   m_ExecutableDir.string(), currentPathError.message());
            }
        }

#if defined(HIMII_DEBUG)
        m_EngineDir = m_ExecutableDir;
        FileSystem::Init(m_ExecutableDir, m_ExecutableDir, true);
#else
        m_EngineDir = m_ExecutableDir / "HimiiEngine";
#ifdef HIMII_PLATFORM_WINDOWS
        const std::wstring engineLibraryDirectory = m_EngineDir.wstring();
        SetDllDirectoryW(engineLibraryDirectory.c_str());
#endif
        FileSystem::Init(m_EngineDir, m_ExecutableDir, false);
#endif

        const std::string engineDirectoryString = m_EngineDir.string();
#ifdef HIMII_PLATFORM_WINDOWS
        if (_putenv_s("HIMII_DIR", engineDirectoryString.c_str()) == 0)
            HIMII_CORE_INFO("Set environment variable HIMII_DIR = {0}", engineDirectoryString);
        else
            HIMII_CORE_ERROR("Failed to set HIMII_DIR environment variable!");
#else
        if (setenv("HIMII_DIR", engineDirectoryString.c_str(), 1) == 0)
            HIMII_CORE_INFO("Set environment variable HIMII_DIR = {0}", engineDirectoryString);
        else
            HIMII_CORE_ERROR("Failed to set HIMII_DIR environment variable!");
#endif
    }

    bool Application::OnWindowClosed(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        HIMII_PROFILE_FUNCTION();

        const uint32_t framebufferWidth = m_Window->GetFramebufferWidth();
        const uint32_t framebufferHeight = m_Window->GetFramebufferHeight();

        if (framebufferWidth == 0 || framebufferHeight == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;
        Renderer::OnWindowResize(framebufferWidth, framebufferHeight);

        (void)e;
        return false;
    }

    void Application::Run()
    {
        HIMII_PROFILE_FUNCTION();

        while (m_Running)
        {
            HIMII_PROFILE_SCOPE("RunLoop")

            float time = (float)glfwGetTime();
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_Minimized)
            {
                JobSystem::PumpMainThreadCompletions();
                {
                    HIMII_PROFILE_SCOPE("LayerStack OnUpdate")
                    for (Layer *layer: m_LayerStack)
                    {
                        layer->OnUpdate(timestep);
                    }
                }

                m_ImGuiLayer->Begin();
                {
                    HIMII_PROFILE_SCOPE("Layerstack OnImGuiRender")
                    for (Layer *layer: m_LayerStack)
                    {
                        layer->OnImGuiRender();
                    }
                }
                m_ImGuiLayer->End();
            }

            m_Window->Update();
            Input::EndFrame();
        }
    }

} // namespace Himii
