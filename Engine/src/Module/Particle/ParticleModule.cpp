#include "Hepch.h"
#include "Module/Particle/ParticleModule.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void ParticleModule::OnUpdate(Timestep timestep)
    {
        if (m_Scene)
            m_Scene->UpdateParticleEmittersAndSystem(timestep);
    }
}
