#include "Hepch.h"
#include "World/World.h"
#include "World/Scene/Scene.h"

#include "Module/Animation/SpriteAnimationModule.h"
#include "Module/Physics/Physics2DModule.h"
#include "Module/Particle/ParticleModule.h"
#include "Module/Render/Renderer/SceneRenderModule.h"
#include "Module/Script/ScriptUpdateModule.h"
#include "Module/Script/ScriptFixedUpdateModule.h"
#include "Module/UserInterface/UserInterfaceModule.h"

namespace Himii
{
    World::~World()
    {
        TearDownModules();
        if (m_ActiveScene)
        {
            m_ActiveScene->SetOwningWorld(nullptr);
            m_ActiveScene = nullptr;
        }
    }

    void World::SetActiveScene(const Ref<Scene> &scene)
    {
        if (m_ActiveScene == scene)
            return;

        TearDownModules();
        if (m_ActiveScene)
            m_ActiveScene->SetOwningWorld(nullptr);

        m_Physics2DWorld = nullptr;
        m_ActiveScene = scene;
        ClearPendingSceneRender();

        if (m_ActiveScene)
        {
            m_ActiveScene->SetOwningWorld(this);
            BuildModulesForActiveScene();
        }
    }

    void World::OnRuntimeStart()
    {
        if (m_ActiveScene)
            m_ActiveScene->OnRuntimeStart();
    }

    void World::OnRuntimeStop()
    {
        if (m_ActiveScene)
            m_ActiveScene->OnRuntimeStop();
    }

    void World::OnSimulationStart()
    {
        if (m_ActiveScene)
            m_ActiveScene->OnSimulationStart();
    }

    void World::OnSimulationStop()
    {
        if (m_ActiveScene)
            m_ActiveScene->OnSimulationStop();
    }

    void World::OnUpdateRuntime(Timestep timestep, bool drawUserInterfaceContent)
    {
        if (!m_ActiveScene)
            return;

        m_Modules.Update(WorldUpdatePhase::UserInterface, timestep);
        m_Modules.Update(WorldUpdatePhase::ScriptUpdate, timestep);
        m_Modules.Update(WorldUpdatePhase::Animation, timestep);
        m_Modules.Update(WorldUpdatePhase::Physics, timestep);
        m_Modules.Update(WorldUpdatePhase::ScriptFixedUpdate, timestep);
        m_Modules.Update(WorldUpdatePhase::Presentation, timestep);

        PrepareRuntimeSceneRender(drawUserInterfaceContent);
        m_Modules.Update(WorldUpdatePhase::Render, timestep);
        ClearPendingSceneRender();
    }

    void World::OnUpdateSimulation(Timestep timestep, EditorCamera &camera)
    {
        if (!m_ActiveScene)
            return;

        // Simulate 保持原次序：Physics → Animation → ScriptFixedUpdate → Render
        m_Modules.Update(WorldUpdatePhase::Physics, timestep);
        m_Modules.Update(WorldUpdatePhase::Animation, timestep);
        m_Modules.Update(WorldUpdatePhase::ScriptFixedUpdate, timestep);

        PrepareSimulationSceneRender(camera);
        m_Modules.Update(WorldUpdatePhase::Render, timestep);
        ClearPendingSceneRender();
    }

    void World::Update(WorldUpdatePhase phase, Timestep timestep)
    {
        m_Modules.Update(phase, timestep);
    }

    void World::PrepareRuntimeSceneRender(bool drawUserInterfaceContent)
    {
        m_PendingSceneRenderKind = PendingSceneRenderKind::RuntimeGameView;
        m_PendingDrawUserInterfaceContent = drawUserInterfaceContent;
        m_PendingEditorCamera = nullptr;
    }

    void World::PrepareSimulationSceneRender(EditorCamera &camera)
    {
        m_PendingSceneRenderKind = PendingSceneRenderKind::SimulationEditorView;
        m_PendingDrawUserInterfaceContent = true;
        m_PendingEditorCamera = &camera;
    }

    void World::ClearPendingSceneRender()
    {
        m_PendingSceneRenderKind = PendingSceneRenderKind::None;
        m_PendingDrawUserInterfaceContent = true;
        m_PendingEditorCamera = nullptr;
    }

    void World::ExecutePendingSceneRender()
    {
        if (!m_ActiveScene)
            return;

        switch (m_PendingSceneRenderKind)
        {
        case PendingSceneRenderKind::RuntimeGameView:
            m_ActiveScene->RenderGameView(
                    m_ActiveScene->GetViewportWidth(),
                    m_ActiveScene->GetViewportHeight(),
                    m_PendingDrawUserInterfaceContent);
            break;
        case PendingSceneRenderKind::SimulationEditorView:
            if (m_PendingEditorCamera)
                m_ActiveScene->RenderSimulationView(*m_PendingEditorCamera);
            break;
        case PendingSceneRenderKind::None:
            break;
        }
    }

    void World::TearDownModules()
    {
        m_Modules.ShutdownAll();
    }

    void World::BuildModulesForActiveScene()
    {
        Scene *scene = m_ActiveScene.get();
        if (!scene)
            return;

        m_Modules.RegisterModule(
                WorldUpdatePhase::UserInterface, CreateScope<UserInterfaceModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::ScriptUpdate, CreateScope<ScriptUpdateModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::Animation, CreateScope<SpriteAnimationModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::Physics, CreateScope<Physics2DModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::ScriptFixedUpdate, CreateScope<ScriptFixedUpdateModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::Presentation, CreateScope<ParticleModule>(scene));
        m_Modules.RegisterModule(
                WorldUpdatePhase::Render, CreateScope<SceneRenderModule>(scene));
        m_Modules.InitializeAll();
    }
}
