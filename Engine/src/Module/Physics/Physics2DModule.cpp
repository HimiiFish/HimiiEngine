#include "Hepch.h"
#include "Module/Physics/Physics2DModule.h"
#include "World/Scene/Scene.h"
#include "World/World.h"

namespace Himii
{
    Physics2DModule::Physics2DModule(Scene *scene)
        : m_Scene(scene)
        , m_Physics2DWorld(scene)
    {
    }

    void Physics2DModule::OnInitialize()
    {
        if (m_Scene && m_Scene->GetOwningWorld())
            m_Scene->GetOwningWorld()->SetPhysics2DWorld(&m_Physics2DWorld);
    }

    void Physics2DModule::OnShutdown()
    {
        m_Physics2DWorld.Stop();
        if (m_Scene && m_Scene->GetOwningWorld())
            m_Scene->GetOwningWorld()->SetPhysics2DWorld(nullptr);
    }

    void Physics2DModule::OnRuntimeStart()
    {
        m_Physics2DWorld.Start();
    }

    void Physics2DModule::OnRuntimeStop()
    {
        m_Physics2DWorld.Stop();
    }

    void Physics2DModule::OnUpdate(Timestep timestep)
    {
        m_Physics2DWorld.Step(timestep);
    }
}
