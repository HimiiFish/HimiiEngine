#include "Hepch.h"
#include "Module/Particle/ParticleEmitterSystem.h"
#include "Module/Particle/ParticleEmitterAsset.h"
#include "Module/Particle/ParticleSystem.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"
#include "World/Scene/Entity.h"
#include "World/Scene/Scene.h"

#include <cmath>

namespace Himii
{
    void ParticleEmitterSystem::UpdateEmittersAndSimulate(
            Scene &scene, ParticleSystem &particleSystem, Timestep timestep)
    {
        if (auto assetManager = ResourceSystem::GetAssetManager())
        {
            auto view = scene.Registry().group<TransformComponent, ParticleEmitterComponent>();
            for (auto entityHandle : view)
            {
                Entity particleEntity = {entityHandle, &scene};
                auto &emitter = particleEntity.GetComponent<ParticleEmitterComponent>();
                if (emitter.EmitterHandle == 0)
                    continue;

                Ref<Asset> assetReference = ResourceSystem::GetAsset(emitter.EmitterHandle);
                if (!assetReference)
                    continue;

                auto emitterAsset = std::static_pointer_cast<ParticleEmitterAsset>(assetReference);
                if (!emitterAsset)
                    continue;

                emitter.EmissionAccumulator += timestep * emitterAsset->EmissionRate;
                int emitCount = static_cast<int>(std::floor(emitter.EmissionAccumulator));
                if (emitCount <= 0)
                    continue;

                emitter.EmissionAccumulator -= static_cast<float>(emitCount);
                ParticleProps particleProperties = emitterAsset->TemplateProps;
                particleProperties.position = scene.GetEntityWorldTranslation(particleEntity);

                for (int index = 0; index < emitCount; ++index)
                    particleSystem.Emit(particleProperties);
            }
        }

        particleSystem.OnUpdate(timestep);
    }
}
