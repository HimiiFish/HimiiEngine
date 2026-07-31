#pragma once

#include <cstdint>

namespace Himii
{
    class Scene;
    class EditorCamera;

    /// 场景世界内容绘制（精灵 / Tilemap / 网格 / 粒子等）。
    class SceneRenderer
    {
    public:
        /// 用 Primary Camera 绘制 Game 世界；无有效相机或尺寸为 0 时返回 false。
        static bool RenderGameWorld(Scene &scene, uint32_t targetWidth, uint32_t targetHeight);

        /// 用编辑相机绘制世界（Editor / Simulate 共用）。
        static void RenderWorld(Scene &scene, EditorCamera &camera);
    };
}
