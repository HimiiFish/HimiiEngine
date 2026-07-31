#pragma once

#include "Resource/Sprite.h"
#include "World/Scene/Components.h"

namespace Himii
{

    class AssetManager;
    class Entity;

    SpriteResolved ResolveSpriteRendererDrawable(Entity entity,
                                                 const SpriteRendererComponent& spriteRenderer,
                                                 AssetManager* assetManager);

    glm::mat4 GetSpriteRendererVisualTransform(const glm::mat4& worldTransform,
                                               const SpriteResolved& resolved);

} // namespace Himii
