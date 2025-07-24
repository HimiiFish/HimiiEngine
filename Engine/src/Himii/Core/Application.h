#pragma once
#include "Himii/Events/Event.h"
#include "Layer.h"
#include "LayerStack.h"
#include "Himii/Core/Window.h"
#include "Himii/ImGui/ImGuiLayer.h"

#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/Buffer.h"
#include "Himii/Renderer/VertexArray.h"

namespace Himii
{
    class Application 
    {
    public:
        Application();
        virtual ~Application();

        void OnEvent(Event &e);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *layer);

        Window &GetWindow()
        {
            return *m_Window;
        }

        void Close();

        /*ImGuiLayer *GetImGuiLayer()
        {
            return m_ImGuiLayer;
        }*/

        static Application &Get()
        {
            return *s_Instance;
        }

        void Run();
        bool OnWindowClosed(WindowCloseEvent &e);
        //bool OnWindowResize(WindowResizeEvent &e);
    private:
        //bool OnWindowClosed(WindowCloseEvent& e);
        Ref<Shader> m_Shader;
        Ref<VertexArray> m_VertexArray;

        Ref<VertexArray> m_SquareVA;
        
    private:
        bool m_Running = true;
        bool m_Minimized = false;

        float m_LastFrameTime = 0.0f;

        LayerStack m_LayerStack;
        Scope<Window> m_Window;
       ImGuiLayer *m_ImGuiLayer;

        static Application *s_Instance;
    };
    Application* CreateApplication();
}