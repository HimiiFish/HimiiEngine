#pragma once

#include "Resource/Sprite.h"

#include <glm/glm.hpp>

namespace Himii
{

    /**
     * 纹理坐标约定（全引擎统一）：
     *
     * - 资源数据（PixelRect、AtlasCoords、.meta）：左上角原点，Y 向下，与 PNG 文件一致。
     * - GPU 纹理（OpenGL + stbi flip on load）：采样坐标 v=0 在底边，v=1 在顶边。
     * - 世界绘制（Renderer2D::DrawQuadUV）：顶点顺序 BL、BR、TR、TL，世界 Y 向上。
     * - ImGui 显示：屏幕 Y 向下；全图用 FullTextureImGuiUvCorners，子区域用 PixelRectToImGuiImageUVCorners。
     *
     * 禁止在业务代码手写 unitU/unitV 或 (0,1) 混用；只调用本类函数。
     */
    struct ImGuiImageUvCorners
    {
        glm::vec2 TopLeft{0.0f, 1.0f};
        glm::vec2 BottomRight{1.0f, 0.0f};
    };

    class SpriteSheetUtility
    {
    public:
        static constexpr ImGuiImageUvCorners FullTextureImGuiUvCorners{};

        /** 逻辑像素矩形：Atlas 网格 (列, 行) + 格边长 → PixelRect（左上原点）。 */
        static glm::ivec4 AtlasGridCoordsToPixelRect(const glm::ivec2& atlasCoordinates,
                                                     uint32_t tilePixelSize);

        /** ImGui::Image / ImageButton 的 uv0、uv1（子区域，视觉正向）。 */
        static ImGuiImageUvCorners PixelRectToImGuiImageUv(const glm::ivec4& pixelRect,
                                                           uint32_t textureWidth,
                                                           uint32_t textureHeight);

        static void PixelRectToImGuiImageUVCorners(const glm::ivec4& pixelRect,
                                                   uint32_t textureWidth,
                                                   uint32_t textureHeight,
                                                   glm::vec2& uvTopLeft,
                                                   glm::vec2& uvBottomRight);

        /**
         * ImGui::GetWindowDrawList()->AddImageQuad 用：屏幕四角 BL、BR、TR、TL 时，
         * 返回对应纹理 UV 顺序 [BL, BR, TR, TL]。
         */
        static std::array<glm::vec2, 4> PixelRectToImGuiQuadUVsForScreenCorners(
            const glm::ivec4& pixelRect,
            uint32_t textureWidth,
            uint32_t textureHeight);

        /** DrawQuadUV / 运行时采样：顶点顺序 BL、BR、TR、TL。 */
        static std::array<glm::vec2, 4> PixelRectToWorldQuadUVs(const glm::ivec4& pixelRect,
                                                                uint32_t textureWidth,
                                                                uint32_t textureHeight);

        static std::array<glm::vec2, 4> AtlasGridCoordsToWorldQuadUVs(const glm::ivec2& atlasCoordinates,
                                                                      uint32_t tilePixelSize,
                                                                      uint32_t textureWidth,
                                                                      uint32_t textureHeight);

        /** 与 PixelRectToWorldQuadUVs 相同；保留别名供旧代码迁移。 */
        static std::array<glm::vec2, 4> PixelRectToUVs(const glm::ivec4& pixelRect,
                                                       uint32_t textureWidth,
                                                       uint32_t textureHeight);

        static std::array<glm::vec2, 4> AtlasGridCoordsToUVs(const glm::ivec2& atlasCoordinates,
                                                             uint32_t tilePixelSize,
                                                             uint32_t textureWidth,
                                                             uint32_t textureHeight)
        {
            return AtlasGridCoordsToWorldQuadUVs(atlasCoordinates, tilePixelSize, textureWidth,
                                                 textureHeight);
        }

        static TextureImportData CreateDefaultSingleSprite(AssetHandle textureHandle,
                                                           uint32_t textureWidth,
                                                           uint32_t textureHeight,
                                                           const std::string& spriteName = "default");

        static void ApplyGridSlice(TextureImportData& importData,
                                   uint32_t textureWidth,
                                   uint32_t textureHeight);

        static glm::mat4 BuildSpriteRenderTransform(const glm::mat4& entityTransform,
                                                    const glm::ivec2& pixelSize,
                                                    uint32_t pixelsPerUnit,
                                                    const glm::vec2& pivot);

        static glm::mat4 ComputeSpriteVisualTransform(const glm::mat4& entityTransform,
                                                      const glm::ivec2& pixelSize,
                                                      uint32_t pixelsPerUnit,
                                                      const glm::vec2& pivot,
                                                      bool useSpriteRegion);

        /** 将 Transform 中的负缩放转为正向缩放 + UV 镜像，避免精灵不可见。 */
        struct SpriteFacingCorrection
        {
            glm::mat4 RenderTransform{1.0f};
            bool FlipUVHorizontal = false;
            bool FlipUVVertical = false;
        };

        static SpriteFacingCorrection CorrectTransformNegativeScaleForSprite(
            const glm::mat4& entityTransform);

        static std::array<glm::vec2, 4> ApplySpriteUvFacing(
            const std::array<glm::vec2, 4>& uvs,
            const SpriteFacingCorrection& correction);
    };

} // namespace Himii
