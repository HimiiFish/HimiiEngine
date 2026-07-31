#pragma once
#include "EngineCore/Core/Timestep.h"
#include "EngineCore/Core/Window.h"
#include "EngineCore/Events/Event.h"
#include "EngineCore/ImGui/ImGuiLayer.h"
#include <filesystem>
#include "Module/ModuleRegistry.h"
#include "Layer.h"
#include "LayerStack.h"

namespace Himii
{
    class World;

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
        Application(const std::string &name = "Himii", ApplicationCommandLineArgs args = ApplicationCommandLineArgs(),
                    const WindowProps &window_props = WindowProps());
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

        const LayerStack &GetLayerStack() const
        {
            return m_LayerStack;
        }

        ModuleRegistry &GetModuleRegistry() { return m_ModuleRegistry; }
        const ModuleRegistry &GetModuleRegistry() const { return m_ModuleRegistry; }

        /// 当前运行时会话（Editor / Runtime 在打开工程后设置）。
        void SetCurrentWorld(const Ref<World> &world) { m_CurrentWorld = world; }
        Ref<World> GetCurrentWorld() const { return m_CurrentWorld; }

        const std::filesystem::path& GetExecutableDir() const { return m_ExecutableDir; }

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
        ModuleRegistry m_ModuleRegistry;
        Ref<World> m_CurrentWorld;
        Scope<Window> m_Window;
        ImGuiLayer *m_ImGuiLayer;
        ApplicationCommandLineArgs m_CommandLineArgs;

        std::filesystem::path m_EngineDir;
        std::filesystem::path m_ExecutableDir;

    private:
        static Application *s_Instance;
    };
    Application *CreateApplication(ApplicationCommandLineArgs args);
} // namespace Himii
