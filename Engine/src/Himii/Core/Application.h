#pragma once
#include "Himii/Events/Event.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Window.h"s

namespace Engine
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

        bool m_Running = true;

        static Window *m_Window;

        static Application *s_Instance;
    };
    Application* CreateApplication();
}