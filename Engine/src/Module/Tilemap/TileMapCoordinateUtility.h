#pragma once

#include "Module/Tilemap/TileMapData.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Himii
{

    struct TileMapEditorGridBounds
    {
        int32_t MinTileX = -8;
        int32_t MinTileY = -8;
        int32_t MaxTileX = 8;
        int32_t MaxTileY = 8;
    };

    // 角对齐：实体局部原点 = tile (0,0) 左下角；网格线与场景默认网格同为整数坐标
    class TileMapCoordinateUtility
    {
    public:
        static constexpr int32_t InvalidTileCoordinate = INT32_MIN;

        static bool IsValidTileCoordinate(const glm::ivec2& tileCoordinates);

        static glm::vec2 TileLocalBottomLeft(int32_t tileX, int32_t tileY, float cellSize);
        static glm::vec2 TileLocalCenter(int32_t tileX, int32_t tileY, float cellSize);

        static glm::ivec2 TileLocalToCoordinates(const glm::vec2& localPosition, float cellSize);

        static TileMapEditorGridBounds GetEditorGridBounds(const TileMapData& mapData,
                                                           int32_t padding = 4);
    };

} // namespace Himii
