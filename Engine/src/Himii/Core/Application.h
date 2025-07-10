#pragma once
#include "Himii/Events/Event.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Himii/Core/Window.h"

namespace Himii
{
    class Application 
    {
    public:
        Application();
        virtual ~Application();

        void Run();
        void OnEvent(Event &e);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *layer);
        inline static Application &Get()
        {
            return *s_Instance;
        }

        LayerStack m_LayerStack;

    private:
        bool OnWindowClosed(WindowCloseEvent& e);

        bool m_Running = true;
        Scope<Window> m_Window;

        static Application *s_Instance;
    };
    Application* CreateApplication();
}