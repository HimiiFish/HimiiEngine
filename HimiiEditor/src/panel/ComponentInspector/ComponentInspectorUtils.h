#pragma once

#include "Resource/Asset.h"
#include "Module/Render/RenderCore/Texture.h"

namespace Himii
{
    void DrawTextureAssignControl(const char* label, Ref<Texture2D>& texture, AssetHandle& textureHandle);
}
