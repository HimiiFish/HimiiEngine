#include "Engine.h"
#include "Himii/Core/EntryPoint.h"
#include "Himii/Project/Project.h"
#include "Himii/Scripting/ScriptEngine.h"

namespace Himii
{
    class RuntimeLayer : public Himii::Layer {
    public:
        RuntimeLayer() : Layer("RuntimeLayer")
        {
        }

        void OnAttach() override
        {
            std::filesystem::path projectFile;
            bool foundProject = false;

            // 遍历当前运行目录
            for (auto &entry: std::filesystem::directory_iterator(std::filesystem::current_path()))
            {
                if (entry.path().extension() == ".hproj")
                {
                    projectFile = entry.path();
                    foundProject = true;
                    break;
                }
            }

            if (!foundProject)
            {
                HIMII_CORE_ERROR("No .hproj file found in current directory!");
                return;
            }

            HIMII_CORE_INFO("Loading Project: {0}", projectFile.string());
            Ref<Project> project = Project::Load(projectFile);

            std::filesystem::path gameDllName = project->GetConfig().ScriptModulePath.filename();
            ScriptEngine::LoadAppAssembly(gameDllName);

            std::filesystem::path startScenePath = Project::GetAssetDirectory() / Project::GetConfig().StartScene;

            if (std::filesystem::exists(startScenePath))
            {
                HIMII_CORE_INFO("Loading Start Scene: {0}", startScenePath.string());
                Ref<Scene> newScene = CreateRef<Scene>();
                SceneSerializer serializer(newScene);
                if (serializer.Deserialize(startScenePath.string()))
                {
                    m_ActiveScene = newScene;
                    m_ActiveScene->OnRuntimeStart();

                    // 强制调整一次视口，防止黑屏
                    auto &window = Application::Get().GetWindow();
                    m_ActiveScene->OnViewportResize(window.GetWidth(), window.GetHeight());
                }
            }
            else
            {
                HIMII_CORE_ERROR("Start scene not found!");
            }
        }

        void OnUpdate(Timestep ts) override
        {

            m_ActiveScene->OnViewportResize(1920, 1080);
            RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
            RenderCommand::Clear();
            m_ActiveScene->OnUpdateRuntime(ts);
        }

        private:
        Ref<Scene> m_ActiveScene;
    };

    class Runtime : public Application {
    public:
        Runtime(const ApplicationCommandLineArgs &args) : Application("Himii Game Engine", args)
        {
            // Runtime 不添加 ImGuiLayer (或者只在 Debug 模式添加)
            // PushOverlay(new ImGuiLayer());

            PushLayer(new RuntimeLayer());
        }

        ~Runtime()
        {
        }
    };

    // 引擎入口点调用此函数创建应用
    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        return new Runtime(args);
    }
} // namespace Himii
