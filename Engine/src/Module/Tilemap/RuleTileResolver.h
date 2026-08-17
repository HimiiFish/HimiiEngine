#pragma once

#include "Module/Tilemap/TileMapData.h"
#include "Module/Tilemap/TileSet.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Himii
{

    struct RuleTileResolveResult
    {
        uint16_t OutputTileIdentifier = 0;
        uint32_t QuarterTurnsClockwise = 0;
        bool MirrorHorizontal = false;
        bool MirrorVertical = false;
        bool HasOutput = false;
    };

    class RuleTileResolver
    {
    public:
        static RuleTileResolveResult Resolve(const TileMapData &mapData,
                                             const TileSet &tileSet,
                                             int32_t tileX,
                                             int32_t tileY,
                                             uint16_t ruleTileIdentifier);

        static void ApplyTextureCoordinateTransform(glm::vec2 textureCoordinates[4],
                                                    uint32_t quarterTurnsClockwise,
                                                    bool mirrorHorizontal,
                                                    bool mirrorVertical);
    };

} // namespace Himii
