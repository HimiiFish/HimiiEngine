#pragma once

#include "EngineCore/Core/Core.h"
#include "EngineCore/Core/Timestep.h"
#include "World/WorldModuleRegistry.h"
#include "World/WorldUpdatePhase.h"

namespace Himii
{
    class Scene;
    class EditorCamera;
    class Physics2DWorld;

    /// 已打开工程的运行时会话：持有世界模块表，并绑定当前活动 Scene。
    class World
    {
    public:
        World() = default;
        ~World();

        World(const World &) = delete;
        World &operator=(const World &) = delete;

        /// 切换活动 Scene：拆掉旧世界模块，按新 Scene 重建并 Initialize。
        void SetActiveScene(const Ref<Scene> &scene);
        Ref<Scene> GetActiveScene() const { return m_ActiveScene; }

        WorldModuleRegistry &GetModuleRegistry() { return m_Modules; }
        const WorldModuleRegistry &GetModuleRegistry() const { return m_Modules; }

        /// 由 Physics2DModule 在 Initialize/Shutdown 时挂接（非拥有）。
        void SetPhysics2DWorld(Physics2DWorld *physics2DWorld) { m_Physics2DWorld = physics2DWorld; }
        Physics2DWorld *GetPhysics2DWorld() const { return m_Physics2DWorld; }

        void OnRuntimeStart();
        void OnRuntimeStop();
        void OnSimulationStart();
        void OnSimulationStop();

        /// Play：按阶段驱动世界模块（含 Render）。
        void OnUpdateRuntime(Timestep timestep, bool drawUserInterfaceContent = true);
        /// Simulate：物理 / FixedUpdate + Render 阶段（编辑相机）。
        void OnUpdateSimulation(Timestep timestep, EditorCamera &camera);

        void Update(WorldUpdatePhase phase, Timestep timestep);

        /// 供 SceneRenderModule 在 Render 阶段执行；由 OnUpdateRuntime / OnUpdateSimulation 预先准备。
        void ExecutePendingSceneRender();

    private:
        enum class PendingSceneRenderKind
        {
            None = 0,
            RuntimeGameView,
            SimulationEditorView
        };

        void TearDownModules();
        void BuildModulesForActiveScene();
        void PrepareRuntimeSceneRender(bool drawUserInterfaceContent);
        void PrepareSimulationSceneRender(EditorCamera &camera);
        void ClearPendingSceneRender();

        Ref<Scene> m_ActiveScene;
        WorldModuleRegistry m_Modules;
        Physics2DWorld *m_Physics2DWorld = nullptr;

        PendingSceneRenderKind m_PendingSceneRenderKind = PendingSceneRenderKind::None;
        bool m_PendingDrawUserInterfaceContent = true;
        EditorCamera *m_PendingEditorCamera = nullptr;
    };
}
