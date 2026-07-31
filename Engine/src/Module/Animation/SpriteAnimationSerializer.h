#pragma once

#include "Module/Animation/SpriteAnimation.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    class SpriteAnimationSerializer
    {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<SpriteAnimation> &animation);
        static Ref<SpriteAnimation> Deserialize(const std::filesystem::path &filepath);
    };
}
