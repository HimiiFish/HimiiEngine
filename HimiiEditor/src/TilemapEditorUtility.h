#pragma once

#include "Module/Tilemap/TileMapData.h"

#include <glm/glm.hpp>

namespace Himii
{

    class TilemapEditorUtility
    {
    public:
        static glm::vec2 GetMapLocalOrigin(const TileMapData& mapData);

        static glm::ivec2 LocalPositionToTileCoordinates(const glm::vec2& localPosition,
                                                         const TileMapData& mapData);

        static glm::vec3 ViewportMouseToWorld(const glm::vec2& viewportMousePosition,
                                              const glm::vec2& viewportSize,
                                              const glm::mat4& viewProjectionMatrix);

        /** 与 Editor 相机一致：射线与 Z = planeZ 平面求交。 */
        static glm::vec3 ViewportMouseToWorldOnPlane(const glm::vec2& viewportMousePosition,
                                                     const glm::vec2& viewportSize,
                                                     const glm::mat4& viewProjectionMatrix,
                                                     float planeZ);

        static glm::vec2 WorldToViewportScreen(const glm::vec3& worldPosition,
                                               const glm::vec2& viewportSize,
                                               const glm::vec2& viewportOrigin,
                                               const glm::mat4& viewProjectionMatrix);

        /** @deprecated 请用 SpriteSheetUtility::PixelRectToImGuiImageUVCorners。 */
        static void AtlasCoordsToImGuiImageUVs(const glm::ivec2& atlasCoordinates,
                                               uint32_t tilePixelSize,
                                               uint32_t textureWidth,
                                               uint32_t textureHeight,
                                               glm::vec2& outUvMin,
                                               glm::vec2& outUvMax);

        static std::array<glm::vec2, 4> AtlasCoordsToImGuiQuadUVs(const glm::ivec2& atlasCoordinates,
                                                                    uint32_t tilePixelSize,
                                                                    uint32_t textureWidth,
                                                                    uint32_t textureHeight);
    };

} // namespace Himii
