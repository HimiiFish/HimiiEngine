#pragma once

#include "EngineCore/Core/Core.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Resource/Asset.h"

namespace Himii
{
    struct BakedEnvironmentLighting
    {
        Ref<TextureCube> EnvironmentCubemap;
        Ref<TextureCube> IrradianceCubemap;
        Ref<TextureCube> PrefilteredCubemap;
        Ref<Texture2D> BrdfLookupTexture;
        bool Valid = false;
    };

    /// Split-Sum IBL：源 HDR → 缓存卷积 → GPU 立方体 / LUT。
    class EnvironmentLightingSystem
    {
    public:
        static void Init();
        static void Shutdown();

        static Ref<Texture2D> GetSharedBrdfLookupTexture();
        static BakedEnvironmentLighting EnsureBaked(AssetHandle environmentMapHandle);
        static void Invalidate(AssetHandle environmentMapHandle);
        /// 检测源 HDR / .meta 变更并丢弃过期缓存（可每帧轻量调用）。
        static void PollSourceChanges();
    };
}
