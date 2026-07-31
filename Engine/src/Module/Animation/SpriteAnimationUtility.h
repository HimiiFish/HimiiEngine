#pragma once

#include "EngineCore/Core/Timestep.h"
#include "World/Scene/Components.h"
#include "Module/Animation/SpriteAnimation.h"

namespace Himii
{

    void ResetSpriteAnimationPlayback(SpriteAnimationComponent& animationComponent);

    void AdvanceSpriteAnimation(SpriteAnimationComponent& animationComponent,
                                const SpriteAnimation& animation,
                                float deltaTimeSeconds);

    const char* SpriteAnimationLoopModeToString(SpriteAnimationLoopMode loopMode);

    SpriteAnimationLoopMode SpriteAnimationLoopModeFromString(const char* text);

} // namespace Himii
