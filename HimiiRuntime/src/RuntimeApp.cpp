#include "Engine.h"
#include "EngineCore/Core/EntryPoint.h"
#include "EngineCore/Core/Input.h"
#include "Project/Project.h"
#include "Module/Script/ScriptEngine.h"
#include "World/World.h"

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
            if (!project)
            {
                HIMII_CORE_ERROR("Failed to load project: {0}", projectFile.string());
                return;
            }

            Project::EnsureSeededDefaultAssets();
            Project::InitializeGameplayDefaultFont();

            if (ImGuiLayer *imguiLayer = Application::Get().GetImGuiLayer())
                imguiLayer->LoadEditorFonts();

            std::filesystem::path gameDllPath =
                    std::filesystem::absolute(Project::GetProjectDirectory() / Project::GetConfig().ScriptModulePath);
            ScriptEngine::LoadAppAssembly(gameDllPath);

            std::filesystem::path startScenePath = Project::GetAssetDirectory() / Project::GetConfig().StartScene;

            m_World = CreateRef<World>();
            Application::Get().SetCurrentWorld(m_World);

            if (std::filesystem::exists(startScenePath))
            {
                HIMII_CORE_INFO("Loading Start Scene: {0}", startScenePath.string());
                Ref<Scene> newScene = CreateRef<Scene>();
                SceneSerializer serializer(newScene);
                if (serializer.Deserialize(startScenePath.string()))
                {
                    m_ActiveScene = newScene;
                    m_World->SetActiveScene(m_ActiveScene);
                    m_World->OnRuntimeStart();

                    auto &window = Application::Get().GetWindow();
                    m_ActiveScene->OnViewportResize(
                            window.GetFramebufferWidth(), window.GetFramebufferHeight());
                }
            }
            else
            {
                HIMII_CORE_ERROR("Start scene not found!");
            }
        }

        void OnUpdate(Timestep ts) override
        {
            if (!m_ActiveScene || !m_World)
                return;

            auto& window = Application::Get().GetWindow();
            const uint32_t framebufferWidth = window.GetFramebufferWidth();
            const uint32_t framebufferHeight = window.GetFramebufferHeight();
            m_ActiveScene->OnViewportResize(framebufferWidth, framebufferHeight);

            Scene::UserInterfacePointerFrameInput userInterfacePointerInput{};
            userInterfacePointerInput.Enabled = true;
            userInterfacePointerInput.HasPosition = true;
            const glm::vec2 mousePosition = Input::GetMousePosition();
            float framebufferCursorX = 0.0f;
            float framebufferCursorY = 0.0f;
            window.MapWindowCursorToFramebufferPixels(
                    mousePosition.x, mousePosition.y, framebufferCursorX, framebufferCursorY);
            // 窗口/帧缓冲光标 Y 向下；UI ortho Y 向上。
            userInterfacePointerInput.PositionInTargetPixels = {
                    framebufferCursorX,
                    static_cast<float>(framebufferHeight) - framebufferCursorY};
            const bool primaryButtonHeld = Input::IsMouseButtonPressed(Mouse::ButtonLeft);
            userInterfacePointerInput.PrimaryButtonHeld = primaryButtonHeld;
            userInterfacePointerInput.PrimaryButtonPressedThisFrame =
                    primaryButtonHeld && !m_PrimaryButtonWasHeld;
            userInterfacePointerInput.PrimaryButtonReleasedThisFrame =
                    !primaryButtonHeld && m_PrimaryButtonWasHeld;
            m_PrimaryButtonWasHeld = primaryButtonHeld;
            m_ActiveScene->SetUserInterfacePointerInput(userInterfacePointerInput);

            RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
            RenderCommand::Clear();
            m_World->OnUpdateRuntime(ts);
        }

        private:
        Ref<World> m_World;
        Ref<Scene> m_ActiveScene;
        bool m_PrimaryButtonWasHeld = false;
    };

    class Runtime : public Application {
    public:
        Runtime(const ApplicationCommandLineArgs &args) : Application("Himii Game Engine", args)
        {
            PushLayer(new RuntimeLayer());
        }

        ~Runtime()
        {
        }
    };

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        return new Runtime(args);
    }
} // namespace Himii
