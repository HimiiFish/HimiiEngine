#include "TilemapEditorUtility.h"

#include "Resource/SpriteSheetUtility.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"

#include <cmath>

namespace Himii
{

    glm::vec2 TilemapEditorUtility::GetMapLocalOrigin(const TileMapData& mapData)
    {
        const TileMapEditorGridBounds gridBounds =
                TileMapCoordinateUtility::GetEditorGridBounds(mapData);
        return TileMapCoordinateUtility::TileLocalBottomLeft(
                gridBounds.MinTileX, gridBounds.MinTileY, mapData.GetCellSize());
    }

    glm::ivec2 TilemapEditorUtility::LocalPositionToTileCoordinates(const glm::vec2& localPosition,
                                                                    const TileMapData& mapData)
    {
        return TileMapCoordinateUtility::TileLocalToCoordinates(localPosition, mapData.GetCellSize());
    }

    glm::vec3 TilemapEditorUtility::ViewportMouseToWorld(const glm::vec2& viewportMousePosition,
                                                         const glm::vec2& viewportSize,
                                                         const glm::mat4& viewProjectionMatrix)
    {
        if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
            return {0.0f, 0.0f, 0.0f};

        return ViewportMouseToWorldOnPlane(viewportMousePosition, viewportSize, viewProjectionMatrix, 0.0f);
    }

    glm::vec3 TilemapEditorUtility::ViewportMouseToWorldOnPlane(const glm::vec2& viewportMousePosition,
                                                                const glm::vec2& viewportSize,
                                                                const glm::mat4& viewProjectionMatrix,
                                                                float planeZ)
    {
        if (viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
            return {0.0f, 0.0f, planeZ};

        const float normalizedX = (viewportMousePosition.x / viewportSize.x) * 2.0f - 1.0f;
        const float normalizedY = (viewportMousePosition.y / viewportSize.y) * 2.0f - 1.0f;

        const glm::mat4 inverseViewProjection = glm::inverse(viewProjectionMatrix);

        glm::vec4 nearPoint = inverseViewProjection * glm::vec4(normalizedX, normalizedY, -1.0f, 1.0f);
        glm::vec4 farPoint = inverseViewProjection * glm::vec4(normalizedX, normalizedY, 1.0f, 1.0f);
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;

        const glm::vec3 rayOrigin = glm::vec3(nearPoint);
        const glm::vec3 rayDirection = glm::normalize(glm::vec3(farPoint - nearPoint));

        if (std::abs(rayDirection.z) < 1.0e-6f)
            return {rayOrigin.x, rayOrigin.y, planeZ};

        const float intersectionDistance = (planeZ - rayOrigin.z) / rayDirection.z;
        return rayOrigin + rayDirection * intersectionDistance;
    }

    glm::vec2 TilemapEditorUtility::WorldToViewportScreen(const glm::vec3& worldPosition,
                                                          const glm::vec2& viewportSize,
                                                          const glm::vec2& viewportOrigin,
                                                          const glm::mat4& viewProjectionMatrix)
    {
        glm::vec4 clipSpace = viewProjectionMatrix * glm::vec4(worldPosition, 1.0f);
        if (clipSpace.w != 0.0f)
            clipSpace /= clipSpace.w;

        const float screenX = (clipSpace.x * 0.5f + 0.5f) * viewportSize.x + viewportOrigin.x;
        const float screenY = viewportOrigin.y + (1.0f - (clipSpace.y * 0.5f + 0.5f)) * viewportSize.y;
        return {screenX, screenY};
    }

    void TilemapEditorUtility::AtlasCoordsToImGuiImageUVs(const glm::ivec2& atlasCoordinates,
                                                            uint32_t tilePixelSize,
                                                            uint32_t textureWidth,
                                                            uint32_t textureHeight,
                                                            glm::vec2& outUvMin,
                                                            glm::vec2& outUvMax)
    {
        const glm::ivec4 pixelRect =
                SpriteSheetUtility::AtlasGridCoordsToPixelRect(atlasCoordinates, tilePixelSize);
        SpriteSheetUtility::PixelRectToImGuiImageUVCorners(
                pixelRect, textureWidth, textureHeight, outUvMin, outUvMax);
    }

    std::array<glm::vec2, 4> TilemapEditorUtility::AtlasCoordsToImGuiQuadUVs(
            const glm::ivec2& atlasCoordinates,
            uint32_t tilePixelSize,
            uint32_t textureWidth,
            uint32_t textureHeight)
    {
        const glm::ivec4 pixelRect =
                SpriteSheetUtility::AtlasGridCoordsToPixelRect(atlasCoordinates, tilePixelSize);
        return SpriteSheetUtility::PixelRectToImGuiQuadUVsForScreenCorners(
                pixelRect, textureWidth, textureHeight);
    }

} // namespace Himii
