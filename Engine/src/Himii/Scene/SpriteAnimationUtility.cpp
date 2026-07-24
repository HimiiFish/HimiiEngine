#include "Hepch.h"
#include "Himii/Scene/SpriteAnimationUtility.h"

#include <algorithm>
#include <cstring>

namespace Himii
{

    void ResetSpriteAnimationPlayback(SpriteAnimationComponent& animationComponent)
    {
        animationComponent.Timer = 0.0f;
        animationComponent.CurrentFrame = 0;
        animationComponent.PlaybackDirection = 1;
    }

    static std::string ResolveActiveAnimationName(const SpriteAnimation& animation,
                                                  const std::string& requestedAnimationName)
    {
        if (!requestedAnimationName.empty() && animation.HasAnimation(requestedAnimationName))
            return requestedAnimationName;

        if (const SpriteAnimationClip* primaryClip = animation.GetPrimaryClip())
            return primaryClip->Name;

        return SpriteAnimationDefaultClipName;
    }

    void AdvanceSpriteAnimation(SpriteAnimationComponent& animationComponent,
                                const SpriteAnimation& animation,
                                float deltaTimeSeconds)
    {
        const std::string activeAnimationName =
                ResolveActiveAnimationName(animation, animationComponent.CurrentAnimationName);

        const int frameCount =
                static_cast<int>(animation.GetFrameCount(activeAnimationName));
        if (frameCount <= 0)
        {
            // 无帧却 Playing=true 时（资产缺失/句柄失效），Once 逻辑永远不会把 Playing 置 false。
            animationComponent.Playing = false;
            animationComponent.Timer = 0.0f;
            return;
        }

        float frameRate = animationComponent.FrameRate > 0.0f
            ? animationComponent.FrameRate
            : animation.GetClipFrameRate(activeAnimationName);
        if (frameRate <= 0.0f)
            frameRate = 1.0f;
        const float frameDuration = 1.0f / frameRate;

        const SpriteAnimationLoopMode loopMode =
                animation.GetClipLoopMode(activeAnimationName);

        // 已停在最后一帧且仍在 Playing 的 Once：补一次结束，避免角色脚本永久卡在攻击态。
        if (loopMode == SpriteAnimationLoopMode::Once
            && animationComponent.CurrentFrame >= frameCount - 1
            && animationComponent.Timer >= frameDuration)
        {
            animationComponent.Playing = false;
            animationComponent.Timer = 0.0f;
            return;
        }

        animationComponent.Timer += deltaTimeSeconds;
        while (animationComponent.Timer >= frameDuration)
        {
            animationComponent.Timer -= frameDuration;

            int nextFrame = animationComponent.CurrentFrame;
            int direction = animationComponent.PlaybackDirection;

            if (loopMode == SpriteAnimationLoopMode::Once)
            {
                if (animationComponent.CurrentFrame >= frameCount - 1)
                {
                    animationComponent.Playing = false;
                    animationComponent.Timer = 0.0f;
                    break;
                }
                nextFrame = animationComponent.CurrentFrame + 1;
            }
            else if (loopMode == SpriteAnimationLoopMode::PingPong)
            {
                nextFrame = animationComponent.CurrentFrame + direction;
                if (nextFrame >= frameCount)
                {
                    nextFrame = frameCount - 1;
                    direction = -1;
                }
                else if (nextFrame < 0)
                {
                    nextFrame = 0;
                    direction = 1;
                }
                animationComponent.PlaybackDirection = direction;
            }
            else
            {
                nextFrame = (animationComponent.CurrentFrame + 1) % frameCount;
            }

            animationComponent.CurrentFrame = nextFrame;
        }
    }

    const char* SpriteAnimationLoopModeToString(SpriteAnimationLoopMode loopMode)
    {
        switch (loopMode)
        {
            case SpriteAnimationLoopMode::Once:
                return "Once";
            case SpriteAnimationLoopMode::PingPong:
                return "PingPong";
            case SpriteAnimationLoopMode::Loop:
            default:
                return "Loop";
        }
    }

    SpriteAnimationLoopMode SpriteAnimationLoopModeFromString(const char* text)
    {
        if (!text)
            return SpriteAnimationLoopMode::Loop;
        if (std::strcmp(text, "Once") == 0)
            return SpriteAnimationLoopMode::Once;
        if (std::strcmp(text, "PingPong") == 0)
            return SpriteAnimationLoopMode::PingPong;
        return SpriteAnimationLoopMode::Loop;
    }

} // namespace Himii
