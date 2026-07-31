#pragma once

#include "EngineCore/Core/Timestep.h"

namespace Himii
{
    class Scene;

    /// 精灵动画播放推进（编辑预览 / Runtime 共用）。
    class SpriteAnimationSystem
    {
    public:
        static void Update(Scene &scene, Timestep timestep, bool allowEditorPreview);
    };
}
