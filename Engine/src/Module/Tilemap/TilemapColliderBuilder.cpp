#include "Hepch.h"
#include "Module/Tilemap/TilemapColliderBuilder.h"

#include "EngineCore/Core/Log.h"
#include "Module/Tilemap/TileMapCoordinateUtility.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace Himii
{

    namespace
    {
        struct CollidableCellKey
        {
            int32_t tileX = 0;
            int32_t tileY = 0;

            bool operator==(const CollidableCellKey& other) const
            {
                return tileX == other.tileX && tileY == other.tileY;
            }
        };

        struct CollidableCellKeyHash
        {
            size_t operator()(const CollidableCellKey& key) const
            {
                const uint64_t packedX = static_cast<uint32_t>(key.tileX);
                const uint64_t packedY = static_cast<uint32_t>(key.tileY);
                return static_cast<size_t>(packedX ^ (packedY << 16));
            }
        };

        struct CollidableRectangle
        {
            int32_t minTileX = 0;
            int32_t minTileY = 0;
            int32_t maxTileX = 0;
            int32_t maxTileY = 0;
        };

        void RecordOrphanTileIdentifier(TilemapColliderBuildReport& report, uint16_t tileIdentifier)
        {
            report.orphanTileIdentifierCount++;
            if (report.sampleOrphanTileIdentifiers.size() < 5)
            {
                if (std::find(report.sampleOrphanTileIdentifiers.begin(),
                              report.sampleOrphanTileIdentifiers.end(),
                              tileIdentifier) == report.sampleOrphanTileIdentifiers.end())
                {
                    report.sampleOrphanTileIdentifiers.push_back(tileIdentifier);
                }
            }
        }

        void CollectCollidableCells(const TileMapData& mapData,
                                    const TileSet& tileSet,
                                    TilemapColliderBuildReport& report,
                                    std::vector<CollidableCellKey>& outCollidableCells)
        {
            mapData.ForEachTile([&](int32_t tileX, int32_t tileY, uint16_t tileIdentifier)
            {
                report.paintedCellCount++;

                const TileDef* tileDefinition = tileSet.GetTileDef(tileIdentifier);
                if (!tileDefinition)
                {
                    RecordOrphanTileIdentifier(report, tileIdentifier);
                    return;
                }

                report.tileDefinitionFoundCount++;

                if (!tileDefinition->Collidable)
                    return;

                report.collidableCellCount++;
                outCollidableCells.push_back({tileX, tileY});
            });
        }

        bool HasCollidableCell(const std::unordered_set<CollidableCellKey, CollidableCellKeyHash>& cells,
                               int32_t tileX,
                               int32_t tileY)
        {
            return cells.find({tileX, tileY}) != cells.end();
        }

        std::vector<CollidableRectangle> MergeCollidableCellsGreedy(
                const std::vector<CollidableCellKey>& collidableCells)
        {
            std::unordered_set<CollidableCellKey, CollidableCellKeyHash> remainingCells(
                    collidableCells.begin(), collidableCells.end());

            std::vector<CollidableRectangle> mergedRectangles;

            while (!remainingCells.empty())
            {
                const CollidableCellKey startCell = *remainingCells.begin();
                const int32_t startTileX = startCell.tileX;
                const int32_t startTileY = startCell.tileY;

                int32_t rectangleWidth = 1;
                while (HasCollidableCell(remainingCells, startTileX + rectangleWidth, startTileY))
                    rectangleWidth++;

                int32_t rectangleHeight = 1;
                bool canGrowHeight = true;
                while (canGrowHeight)
                {
                    for (int32_t offsetX = 0; offsetX < rectangleWidth; ++offsetX)
                    {
                        if (!HasCollidableCell(remainingCells, startTileX + offsetX,
                                               startTileY + rectangleHeight))
                        {
                            canGrowHeight = false;
                            break;
                        }
                    }
                    if (canGrowHeight)
                        rectangleHeight++;
                }

                for (int32_t offsetY = 0; offsetY < rectangleHeight; ++offsetY)
                {
                    for (int32_t offsetX = 0; offsetX < rectangleWidth; ++offsetX)
                        remainingCells.erase({startTileX + offsetX, startTileY + offsetY});
                }

                CollidableRectangle rectangle;
                rectangle.minTileX = startTileX;
                rectangle.minTileY = startTileY;
                rectangle.maxTileX = startTileX + rectangleWidth - 1;
                rectangle.maxTileY = startTileY + rectangleHeight - 1;
                mergedRectangles.push_back(rectangle);
            }

            return mergedRectangles;
        }

        uint32_t CreateShapeForTileCell(b2BodyId bodyId,
                                        const TransformComponent& transform,
                                        float cellSize,
                                        float halfWidth,
                                        float halfHeight,
                                        const b2Rot& shapeRotation,
                                        int32_t tileX,
                                        int32_t tileY)
        {
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.enableContactEvents = true;

            const glm::vec2 tileCenter =
                    TileMapCoordinateUtility::TileLocalCenter(tileX, tileY, cellSize);
            const b2Polygon polygon = b2MakeOffsetBox(
                    halfWidth, halfHeight,
                    {tileCenter.x * transform.Scale.x, tileCenter.y * transform.Scale.y},
                    shapeRotation);

            const b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
            return b2Shape_IsValid(shapeId) ? 1u : 0u;
        }

        uint32_t CreateShapeForMergedRectangle(b2BodyId bodyId,
                                               const TransformComponent& transform,
                                               float cellSize,
                                               const b2Rot& shapeRotation,
                                               const CollidableRectangle& rectangle)
        {
            const float halfWidth =
                    static_cast<float>(rectangle.maxTileX - rectangle.minTileX + 1)
                    * cellSize * transform.Scale.x * 0.5f;
            const float halfHeight =
                    static_cast<float>(rectangle.maxTileY - rectangle.minTileY + 1)
                    * cellSize * transform.Scale.y * 0.5f;

            const glm::vec2 bottomLeft = TileMapCoordinateUtility::TileLocalBottomLeft(
                    rectangle.minTileX, rectangle.minTileY, cellSize);
            const glm::vec2 topRight = TileMapCoordinateUtility::TileLocalBottomLeft(
                    rectangle.maxTileX + 1, rectangle.maxTileY + 1, cellSize);

            const glm::vec2 localCenter = (bottomLeft + topRight) * 0.5f;

            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.enableContactEvents = true;

            const b2Polygon polygon = b2MakeOffsetBox(
                    halfWidth, halfHeight,
                    {localCenter.x * transform.Scale.x, localCenter.y * transform.Scale.y},
                    shapeRotation);

            const b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
            return b2Shape_IsValid(shapeId) ? 1u : 0u;
        }
    }

    bool TilemapColliderBuilder::ShouldCellCollide(uint16_t tileIdentifier, const TileSet& tileSet)
    {
        const TileDef* tileDefinition = tileSet.GetTileDef(tileIdentifier);
        return tileDefinition && tileDefinition->Collidable;
    }

    TilemapColliderBuildReport TilemapColliderBuilder::AnalyzeCollidableCells(
            const TileMapData& mapData,
            const TileSet& tileSet)
    {
        TilemapColliderBuildReport report;
        std::vector<CollidableCellKey> collidableCells;
        CollectCollidableCells(mapData, tileSet, report, collidableCells);
        report.shapeCreatedCount = report.collidableCellCount;
        return report;
    }

    TilemapColliderBuildReport TilemapColliderBuilder::CreateColliderShapes(
            b2WorldId world,
            const TransformComponent& transform,
            const TileMapData& mapData,
            const TileSet& tileSet,
            void* bodyUserData,
            bool mergeAdjacentCells)
    {
        TilemapColliderBuildReport report;

        const float cellSize = mapData.GetCellSize();
        if (cellSize <= 0.0f)
            return report;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {transform.Position.x, transform.Position.y};
        bodyDef.rotation = b2MakeRot(transform.Rotation.z);
        bodyDef.userData = bodyUserData;

        const b2BodyId bodyId = b2CreateBody(world, &bodyDef);
        if (!b2Body_IsValid(bodyId))
            return report;

        const float halfWidth = cellSize * transform.Scale.x * 0.5f;
        const float halfHeight = cellSize * transform.Scale.y * 0.5f;
        const b2Rot shapeRotation = b2MakeRot(0.0f);

        std::vector<CollidableCellKey> collidableCells;
        CollectCollidableCells(mapData, tileSet, report, collidableCells);

        if (mergeAdjacentCells)
        {
            const std::vector<CollidableRectangle> mergedRectangles =
                    MergeCollidableCellsGreedy(collidableCells);
            for (const CollidableRectangle& rectangle : mergedRectangles)
            {
                report.shapeCreatedCount += CreateShapeForMergedRectangle(
                        bodyId, transform, cellSize, shapeRotation, rectangle);
            }
        }
        else
        {
            for (const CollidableCellKey& cell : collidableCells)
            {
                report.shapeCreatedCount += CreateShapeForTileCell(
                        bodyId, transform, cellSize, halfWidth, halfHeight, shapeRotation,
                        cell.tileX, cell.tileY);
            }
        }

        return report;
    }

    void TilemapColliderBuilder::LogBuildReport(const std::string& entityName,
                                                const TilemapColliderBuildReport& report)
    {
        if (report.paintedCellCount == 0)
        {
            HIMII_CORE_WARNING(
                    "TilemapCollider2D: entity '{0}' has no painted tiles.",
                    entityName);
            return;
        }

        if (report.orphanTileIdentifierCount > 0)
        {
            std::ostringstream orphanStream;
            for (size_t index = 0; index < report.sampleOrphanTileIdentifiers.size(); ++index)
            {
                if (index > 0)
                    orphanStream << ", ";
                orphanStream << report.sampleOrphanTileIdentifiers[index];
            }

            HIMII_CORE_WARNING(
                    "TilemapCollider2D: entity '{0}' has {1} cells with unknown tile IDs"
                    " (examples: {2}). Re-slice TileSet or repaint those cells.",
                    entityName,
                    report.orphanTileIdentifierCount,
                    orphanStream.str());
        }

        if (report.collidableCellCount == 0)
        {
            HIMII_CORE_WARNING(
                    "TilemapCollider2D: entity '{0}' has {1} painted cells but 0 Collidable cells."
                    " In TileMap Setup, select tile types, enable Collidable, and Save TileSet.",
                    entityName,
                    report.paintedCellCount);
            return;
        }

        if (report.shapeCreatedCount == 0)
        {
            HIMII_CORE_WARNING(
                    "TilemapCollider2D: entity '{0}' resolved {1} collidable cells but created 0 shapes.",
                    entityName,
                    report.collidableCellCount);
        }
    }

} // namespace Himii
