#include "Hepch.h"
#include "Module/Render/SpriteRendererUtility.h"

#include "Resource/AssetManager.h"
#include "Resource/SpriteSheetUtility.h"
#include "World/Scene/Entity.h"
#include "Module/Animation/SpriteAnimation.h"

namespace Himii
{

    static SpriteResolved ResolveAnimationFrameDrawable(const SpriteAnimation& animation,
                                                        const std::string& animationName,
                                                        int frameIndex,
                                                        AssetManager* assetManager)
    {
        if (!assetManager || animation.GetFrameCount(animationName) == 0)
            return {};

        if (animation.UsesAtlasFrames(animationName))
        {
            const glm::ivec2 atlasCoordinates =
                    animation.GetAtlasFrameCoordinates(animationName, frameIndex);
            const AssetHandle atlasTextureHandle = animation.GetAtlasTextureHandle();
            const uint32_t gridCellSize = animation.GetAtlasGridCellSize();

            Ref<Asset> atlasAsset = assetManager->GetAsset(atlasTextureHandle);
            if (!atlasAsset)
                return {};

            Ref<Texture2D> atlasTexture = std::static_pointer_cast<Texture2D>(atlasAsset);

            SpriteResolved resolved;
            resolved.Texture = atlasTexture;
            resolved.UVs = SpriteSheetUtility::AtlasGridCoordsToUVs(
                    atlasCoordinates, gridCellSize,
                    atlasTexture->GetWidth(), atlasTexture->GetHeight());
            resolved.Pivot = {0.5f, 0.5f};
            resolved.PixelSize = {static_cast<int>(gridCellSize), static_cast<int>(gridCellSize)};

            const TextureImportData* importData =
                    assetManager->GetTextureImportData(atlasTextureHandle);
            resolved.PixelsPerUnit =
                    importData && importData->PixelsPerUnit > 0 ? importData->PixelsPerUnit : 100;
            resolved.IsValid = true;
            return resolved;
        }

        const AssetHandle frameHandle = animation.GetFrame(animationName, frameIndex);
        if (frameHandle == 0)
            return {};

        if (assetManager->IsSpriteHandle(frameHandle))
            return assetManager->ResolveSprite(frameHandle);

        if (assetManager->IsAssetHandleValid(frameHandle))
        {
            const AssetHandle defaultSpriteHandle =
                    assetManager->GetDefaultSpriteHandleForTexture(frameHandle);
            if (defaultSpriteHandle != 0)
                return assetManager->ResolveSprite(defaultSpriteHandle);
        }

        return {};
    }

    SpriteResolved ResolveSpriteRendererDrawable(Entity entity,
                                                 const SpriteRendererComponent& spriteRenderer,
                                                 AssetManager* assetManager)
    {
        if (!assetManager)
            return {};

        if (entity && entity.HasComponent<SpriteAnimationComponent>())
        {
            const SpriteAnimationComponent& animationComponent =
                    entity.GetComponent<SpriteAnimationComponent>();

            if (animationComponent.AnimationHandle != 0
                && assetManager->IsAssetHandleValid(animationComponent.AnimationHandle))
            {
                Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                        assetManager->GetAsset(animationComponent.AnimationHandle));

                if (animation)
                {
                    std::string activeAnimationName = animationComponent.CurrentAnimationName;
                    if (activeAnimationName.empty()
                        || !animation->HasAnimation(activeAnimationName))
                    {
                        if (const SpriteAnimationClip* primaryClip = animation->GetPrimaryClip())
                            activeAnimationName = primaryClip->Name;
                    }

                    if (animation->GetFrameCount(activeAnimationName) > 0)
                    {
                        const int frameIndex = animationComponent.CurrentFrame >= 0
                            ? animationComponent.CurrentFrame
                            : 0;
                        const int clampedFrameIndex = frameIndex % static_cast<int>(
                                animation->GetFrameCount(activeAnimationName));

                        SpriteResolved animationResolved = ResolveAnimationFrameDrawable(
                                *animation, activeAnimationName, clampedFrameIndex, assetManager);
                        if (animationResolved.IsValid)
                            return animationResolved;
                    }
                }
            }
        }

        if (spriteRenderer.SpriteAssetHandle != 0)
            return assetManager->ResolveSprite(spriteRenderer.SpriteAssetHandle);

        return {};
    }

    glm::mat4 GetSpriteRendererVisualTransform(const glm::mat4& worldTransform,
                                               const SpriteResolved& resolved)
    {
        if (!resolved.IsValid)
            return worldTransform;

        return SpriteSheetUtility::BuildSpriteRenderTransform(
                worldTransform, resolved.PixelSize, resolved.PixelsPerUnit, resolved.Pivot);
    }

} // namespace Himii
