#include "Hepch.h"
#include "Himii/Core/Application.h"
#include "Himii/Renderer/Renderer.h"
#include <GLFW/glfw3.h>


#include "Himii/Scripting/ScriptEngine.h"

namespace Himii
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const std::string &name, ApplicationCommandLineArgs args) : m_CommandLineArgs(args)
    {
        HIMII_PROFILE_FUNCTION();


        SetEnvironmentVariables();
        s_Instance = this;
        m_Window = Window::Create(WindowProps(name));
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        Renderer::Init();
        ScriptEngine::Init();

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        HIMII_PROFILE_FUNCTION();

        ScriptEngine::Shutdown();
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
        //获取引擎根目录
        std::string engineDir;

#if defined(HIMII_DEBUG) && defined(HIMII_ROOT_DIR)
        // 开发模式：直接指向源码目录 E:\HimiiEngine
        engineDir = HIMII_ROOT_DIR;
#else
        // 发布模式：假设结构是 Bin/HimiiEditor.exe，那么根目录就是上一级
        // 注意：这取决于你的安装目录结构，假设 Release 包解压后就是根目录
        auto exePath = std::filesystem::current_path(); // 或者用 GetModuleFileName 获取更准
        engineDir = exePath.string();
        // 如果 exe 在 bin 下，可能需要 engineDir = exePath.parent_path().string();
#endif

        // 2. 设置环境变量 HIMII_DIR
        // Windows 专用 API，跨平台可以用 setenv
        // 格式：变量名，变量值
        m_EngineDir = engineDir;
#ifdef HIMII_PLATFORM_WINDOWS
        if (_putenv_s("HIMII_DIR", engineDir.c_str()) == 0)
        {
            HIMII_CORE_INFO("Set environment variable HIMII_DIR = {0}", engineDir);
        }
        else
        {
            HIMII_CORE_ERROR("Failed to set HIMII_DIR environment variable!");
        }
#else
        // Linux/Unix implementation
        if (setenv("HIMII_DIR", engineDir.c_str(), 1) == 0)
        {
             HIMII_CORE_INFO("Set environment variable HIMII_DIR = {0}", engineDir);
        }
        else
        {
             HIMII_CORE_ERROR("Failed to set HIMII_DIR environment variable!");
        }
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

        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_Minimized = true;
            return false;
        }

        m_Minimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

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
                {
                    HIMII_PROFILE_SCOPE("LayerStack OnUpdate")
                    // Layer Update
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

            //Window Update
            m_Window->Update();
        }
    }

} // namespace Himii
