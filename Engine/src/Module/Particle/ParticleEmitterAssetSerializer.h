#pragma once

#include "Module/Particle/ParticleEmitterAsset.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    class ParticleEmitterAssetSerializer
    {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<ParticleEmitterAsset> &asset);
        static Ref<ParticleEmitterAsset> Deserialize(const std::filesystem::path &filepath);
    };
}
