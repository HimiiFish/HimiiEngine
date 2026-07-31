#include "Hepch.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"

#include <cmath>

namespace Himii
{

    bool TileMapCoordinateUtility::IsValidTileCoordinate(const glm::ivec2& tileCoordinates)
    {
        return tileCoordinates.x != InvalidTileCoordinate;
    }

    glm::vec2 TileMapCoordinateUtility::TileLocalBottomLeft(int32_t tileX, int32_t tileY,
                                                            float cellSize)
    {
        // 角对齐：tile (0,0) 左下角在实体原点，与编辑器场景网格（整数坐标线）一致
        return {
            static_cast<float>(tileX) * cellSize,
            static_cast<float>(tileY) * cellSize};
    }

    glm::vec2 TileMapCoordinateUtility::TileLocalCenter(int32_t tileX, int32_t tileY,
                                                        float cellSize)
    {
        const glm::vec2 bottomLeft = TileLocalBottomLeft(tileX, tileY, cellSize);
        return bottomLeft + glm::vec2(cellSize * 0.5f, cellSize * 0.5f);
    }

    glm::ivec2 TileMapCoordinateUtility::TileLocalToCoordinates(const glm::vec2& localPosition,
                                                                float cellSize)
    {
        if (cellSize <= 0.0f)
            return {InvalidTileCoordinate, InvalidTileCoordinate};

        const int32_t tileX =
                static_cast<int32_t>(std::floor(localPosition.x / cellSize));
        const int32_t tileY =
                static_cast<int32_t>(std::floor(localPosition.y / cellSize));
        return {tileX, tileY};
    }

    TileMapEditorGridBounds TileMapCoordinateUtility::GetEditorGridBounds(
            const TileMapData& mapData, int32_t padding)
    {
        TileMapEditorGridBounds bounds;
        if (!mapData.HasBounds())
            return bounds;

        int32_t minTileX = 0;
        int32_t minTileY = 0;
        int32_t maxTileX = 0;
        int32_t maxTileY = 0;
        mapData.GetBounds(minTileX, minTileY, maxTileX, maxTileY);

        bounds.MinTileX = minTileX - padding;
        bounds.MinTileY = minTileY - padding;
        bounds.MaxTileX = maxTileX + padding;
        bounds.MaxTileY = maxTileY + padding;
        return bounds;
    }

} // namespace Himii
