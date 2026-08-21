#pragma once

#include "EngineCore/Core/Core.h"

namespace Himii
{
    class Framebuffer;

    /// 全屏 HDR → Exposure → ACES → LDR 显示编码。
    class SceneColorResolvePass
    {
    public:
        static void Init();
        static void Shutdown();

        /// 采样 sourceFramebuffer 的 color0（RGBA16F），写入当前已绑定的 LDR 目标。
        static void Resolve(const Ref<Framebuffer> &sourceFramebuffer, float exposure);

        /// 无主相机时使用的默认曝光。
        static constexpr float DefaultExposure = 1.0f;
        static float ClampExposure(float exposure);
    };
}
