#pragma once
#include "Himii/Core/Core.h"
#include <filesystem>
#include "Himii/Scene/SpriteAnimation.h"

namespace Himii
{

    class SpriteAnimationSerializer {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<SpriteAnimation> &animation);
        static Ref<SpriteAnimation> Deserialize(const std::filesystem::path &filepath);
    };

} // namespace Himii
