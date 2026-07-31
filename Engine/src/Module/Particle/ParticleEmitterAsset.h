#pragma once

#include "Resource/Asset.h"
#include "Module/Particle/ParticleSystem.h"

namespace Himii
{
    // 粒子发射器资源：可序列化为 .particle，由 ParticleEmitterComponent 引用
    class ParticleEmitterAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::ParticleEmitter; }

        ParticleProps TemplateProps;
        float EmissionRate = 10.0f;
        bool Looping = true;
    };
}
