#include "Hepch.h"
#include "Himii/Core/Application.h"
#include "Himii/Renderer/Renderer.h"
#include <GLFW/glfw3.h>


namespace Himii
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const std::string &name, ApplicationCommandLineArgs args) : m_CommandLineArgs(args)
    {
        HIMII_PROFILE_FUNCTION();

        s_Instance = this;
        m_Window = Window::Create(WindowProps(name));
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        Renderer::Init();
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        HIMII_PROFILE_FUNCTION();
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
