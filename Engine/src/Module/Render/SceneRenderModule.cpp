#include "Hepch.h"
#include "Module/Render/SceneRenderModule.h"
#include "World/Scene/Scene.h"
#include "World/World.h"

namespace Himii
{
    void SceneRenderModule::OnUpdate(Timestep timestep)
    {
        (void)timestep;
        if (!m_Scene)
            return;

        World *world = m_Scene->GetOwningWorld();
        if (world)
            world->ExecutePendingSceneRender();
    }
}
