#pragma once
#include "Himii/Core/Timestep.h"
#include "Himii/Core/Window.h"
#include "Himii/Events/Event.h"
#include "Himii/ImGui/ImGuiLayer.h"
#include "Himii/Renderer/Buffer.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/VertexArray.h"
#include <filesystem>
#include "Layer.h"
#include "LayerStack.h"

namespace Himii
{
    struct ApplicationCommandLineArgs {
        int Count = 0;
        char **Args = nullptr;

        const char *operator[](int index) const
        {
            HIMII_CORE_ASSERT(index < Count);
            return Args[index];
        }
    };

    class Application {
    public:
        Application(const std::string &name = "Himii", ApplicationCommandLineArgs args = ApplicationCommandLineArgs());
        virtual ~Application();

        void OnEvent(Event &e);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *layer);

        Window &GetWindow()
        {
            return *m_Window;
        }

        void Close();

        ImGuiLayer *GetImGuiLayer()
        {
            return m_ImGuiLayer;
        }

        static Application &Get()
        {
            return *s_Instance;
        }

        ApplicationCommandLineArgs GetCommandLineArgs() const
        {
            return m_CommandLineArgs;
        }


        void Run();

        // 暴露 LayerStack 只读访问（用于层间简单通信）
        const LayerStack &GetLayerStack() const
        {
            return m_LayerStack;
        }

        const static std::filesystem::path &GetEngineDir()
        {
            return s_Instance->m_EngineDir;
        }

    private:
        void SetEnvironmentVariables();
        bool OnWindowClosed(WindowCloseEvent &e);
        bool OnWindowResize(WindowResizeEvent &e);

    private:
        bool m_Running = true;
        bool m_Minimized = false;
        float m_LastFrameTime = 0.0f;

        LayerStack m_LayerStack;
        Scope<Window> m_Window;
        ImGuiLayer *m_ImGuiLayer;
        ApplicationCommandLineArgs m_CommandLineArgs;

        std::filesystem::path m_EngineDir;

    private:
        static Application *s_Instance;
    };
    Application *CreateApplication(ApplicationCommandLineArgs args);
} // namespace Himii
