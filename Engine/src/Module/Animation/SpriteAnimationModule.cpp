#include "Hepch.h"
#include "Module/Animation/SpriteAnimationModule.h"
#include "Module/Animation/SpriteAnimationSystem.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void SpriteAnimationModule::OnUpdate(Timestep timestep)
    {
        if (m_Scene)
            SpriteAnimationSystem::Update(*m_Scene, timestep, false);
    }
}
