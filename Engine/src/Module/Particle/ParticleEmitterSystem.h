#pragma once

#include "EngineCore/Core/Timestep.h"

namespace Himii
{
    class Scene;
    class ParticleSystem;

    /// 粒子发射器驱动与粒子系统步进。
    class ParticleEmitterSystem
    {
    public:
        static void UpdateEmittersAndSimulate(
                Scene &scene, ParticleSystem &particleSystem, Timestep timestep);
    };
}
