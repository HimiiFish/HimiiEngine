#pragma once

#include "World/Scene/Components.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Tilemap/TileSet.h"

#include "box2d/box2d.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Himii
{

    struct TilemapColliderBuildReport
    {
        uint32_t paintedCellCount = 0;
        uint32_t tileDefinitionFoundCount = 0;
        uint32_t collidableCellCount = 0;
        uint32_t orphanTileIdentifierCount = 0;
        uint32_t shapeCreatedCount = 0;
        std::vector<uint16_t> sampleOrphanTileIdentifiers;
    };

    class TilemapColliderBuilder
    {
    public:
        static bool ShouldCellCollide(uint16_t tileIdentifier, const TileSet& tileSet);

        static TilemapColliderBuildReport AnalyzeCollidableCells(const TileMapData& mapData,
                                                                   const TileSet& tileSet);

        static TilemapColliderBuildReport CreateColliderShapes(
                b2WorldId world,
                const TransformComponent& transform,
                const TileMapData& mapData,
                const TileSet& tileSet,
                void* bodyUserData,
                bool mergeAdjacentCells);

        static void LogBuildReport(const std::string& entityName,
                                   const TilemapColliderBuildReport& report);
    };

} // namespace Himii
