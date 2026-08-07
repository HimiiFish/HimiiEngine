#pragma once

#include "Module/Render/RenderCore/Texture.h"
#include "EngineCore/Core/Core.h"
#include "Resource/Asset.h"

namespace Himii
{
    class AssetManager;

    /// Content 材质球缩略图：GPU 离屏渲染；未就绪时返回空，由调用方显示占位图标。
    Ref<Texture2D> GetOrCreateMaterialThumbnail(AssetManager *assetManager, AssetHandle materialHandle);

    /// 每帧处理排队中的缩略图生成（默认每帧最多 2 张）。
    void ProcessPendingMaterialThumbnails(AssetManager *assetManager, uint32_t maxGenerationsPerFrame = 2);

    void InvalidateMaterialThumbnail(AssetHandle materialHandle);
    void ClearMaterialThumbnailCache();
}
