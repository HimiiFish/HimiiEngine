#include "Hepch.h"
#include "EngineCore/Core/Application.h"
#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/Input.h"
#include "EngineCore/Core/JobSystem.h"
#include "EngineCore/Utils/PlatformClock.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "Module/Audio/AudioModule.h"
#include "Module/Render/Renderer/RenderModule.h"
#include "Module/Resource/ResourceModule.h"
#include "Module/Script/ScriptModule.h"
#include "World/World.h"

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

        // Resource 需尽早注册：Shutdown 时 Unbind；AssetManager 仍由 Project Bind。
        m_ModuleRegistry.RegisterModule(CreateScope<ResourceModule>());
        // Render 需在 Audio 前注册：窗口已创建，InitializeAll 时完成图形初始化。
        m_ModuleRegistry.RegisterModule(CreateScope<RenderModule>());
        m_ModuleRegistry.RegisterModule(CreateScope<AudioModule>());
        m_ModuleRegistry.RegisterModule(CreateScope<ScriptModule>());
        m_ModuleRegistry.InitializeAll();
        RenderModule::OnWindowResize(m_Window->GetFramebufferWidth(), m_Window->GetFramebufferHeight());

        JobSystem::Initialize();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        HIMII_PROFILE_FUNCTION();

        m_ModuleRegistry.ShutdownAll();

        JobSystem::Shutdown();
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

        if (e.GetEventType() == EventType::WindowClose)
        {
            for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
            {
                if (e.Handled)
                    break;
                (*it)->OnEvent(e);
            }

            if (!e.Handled)
                m_Running = false;
            return;
        }

        EventDispatcher dispatcher(e);
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
        m_ExecutableDir = PlatformProcess::GetExecutableDirectory();

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
        PlatformProcess::SetDynamicLibrarySearchDirectory(m_EngineDir);
        FileSystem::Init(m_EngineDir, m_ExecutableDir, false);
#endif

        const std::string engineDirectoryString = m_EngineDir.string();
        if (PlatformProcess::SetEnvironmentVariableValue("HIMII_DIR", engineDirectoryString))
            HIMII_CORE_INFO("Set environment variable HIMII_DIR = {0}", engineDirectoryString);
        else
            HIMII_CORE_ERROR("Failed to set HIMII_DIR environment variable!");
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
        RenderModule::OnWindowResize(framebufferWidth, framebufferHeight);

        (void)e;
        return false;
    }

    void Application::Run()
    {
        HIMII_PROFILE_FUNCTION();

        while (m_Running)
        {
            HIMII_PROFILE_SCOPE("RunLoop")

            float time = static_cast<float>(PlatformClock::GetTimeSeconds());
            Timestep timestep = time - m_LastFrameTime;
            m_LastFrameTime = time;

            if (!m_Minimized)
            {
                JobSystem::PumpMainThreadCompletions();
                m_ModuleRegistry.UpdateAll(timestep);
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
