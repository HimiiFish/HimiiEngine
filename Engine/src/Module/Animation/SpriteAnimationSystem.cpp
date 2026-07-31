#include "Hepch.h"
#include "Module/Animation/SpriteAnimationSystem.h"
#include "Module/Animation/SpriteAnimation.h"
#include "Module/Animation/SpriteAnimationUtility.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"
#include "World/Scene/Scene.h"

namespace Himii
{
    void SpriteAnimationSystem::Update(Scene &scene, Timestep timestep, bool allowEditorPreview)
    {
        auto view = scene.Registry().view<SpriteAnimationComponent, SpriteRendererComponent>();
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return;

        const float deltaTimeSeconds = timestep.GetSeconds();

        for (auto entityHandle : view)
        {
            auto [animationComponent, spriteRendererComponent] =
                    view.get<SpriteAnimationComponent, SpriteRendererComponent>(entityHandle);
            (void)spriteRendererComponent;

            if (animationComponent.AnimationHandle == 0
                || !ResourceSystem::IsAssetHandleValid(animationComponent.AnimationHandle))
                continue;

            Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                    ResourceSystem::GetAsset(animationComponent.AnimationHandle));
            if (!animation)
                continue;

            if (animationComponent.CurrentAnimationName.empty()
                || !animation->HasAnimation(animationComponent.CurrentAnimationName))
            {
                if (const SpriteAnimationClip *primaryClip = animation->GetPrimaryClip())
                    animationComponent.CurrentAnimationName = primaryClip->Name;
            }

            if (animation->GetFrameCount(animationComponent.CurrentAnimationName) == 0)
                continue;

            const bool shouldAdvance =
                    animationComponent.Playing
                    || (allowEditorPreview && animationComponent.PreviewInScene);

            if (shouldAdvance)
                AdvanceSpriteAnimation(animationComponent, *animation, deltaTimeSeconds);
        }
    }
}
