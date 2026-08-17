#include "Hepch.h"
#include "Module/Tilemap/RuleTileResolver.h"

#include <algorithm>

namespace Himii
{

    namespace
    {
        constexpr uint32_t DefaultOutputRuleIndexSentinel = 0xFFFFFFFFu;

        const glm::ivec2 NeighborOffsets[RuleTileNeighborCount] = {
                {0, 1},   // North
                {1, 1},   // NorthEast
                {1, 0},   // East
                {1, -1},  // SouthEast
                {0, -1},  // South
                {-1, -1}, // SouthWest
                {-1, 0},  // West
                {-1, 1}   // NorthWest
        };

        uint32_t ComputeStableHash(int32_t tileX, int32_t tileY, uint32_t ruleIndex)
        {
            uint32_t hash = 2166136261u;
            auto mix = [&hash](uint32_t value)
            {
                hash ^= value;
                hash *= 16777619u;
            };
            mix(static_cast<uint32_t>(tileX));
            mix(static_cast<uint32_t>(tileY));
            mix(ruleIndex);
            return hash;
        }

        bool IsValidOutputTileIdentifier(const TileSet &tileSet, uint16_t tileIdentifier)
        {
            return tileSet.GetTileDef(tileIdentifier) != nullptr
                    && tileSet.GetRuleTileDefinition(tileIdentifier) == nullptr;
        }

        uint16_t PickOutputTileIdentifier(const TileSet &tileSet,
                                          const std::vector<uint16_t> &outputTileIdentifiers,
                                          int32_t tileX,
                                          int32_t tileY,
                                          uint32_t ruleIndex)
        {
            std::vector<uint16_t> validOutputTileIdentifiers;
            validOutputTileIdentifiers.reserve(outputTileIdentifiers.size());
            for (uint16_t outputTileIdentifier : outputTileIdentifiers)
            {
                if (IsValidOutputTileIdentifier(tileSet, outputTileIdentifier))
                    validOutputTileIdentifiers.push_back(outputTileIdentifier);
            }

            if (validOutputTileIdentifiers.empty())
                return 0;

            if (validOutputTileIdentifiers.size() == 1)
                return validOutputTileIdentifiers[0];

            const uint32_t variantIndex =
                    ComputeStableHash(tileX, tileY, ruleIndex)
                    % static_cast<uint32_t>(validOutputTileIdentifiers.size());
            return validOutputTileIdentifiers[variantIndex];
        }

        bool NeighborConditionsMatch(const RuleTileNeighborCondition neighborConditions[RuleTileNeighborCount],
                                     const uint16_t neighborTileIdentifiers[RuleTileNeighborCount],
                                     uint16_t ruleTileIdentifier)
        {
            for (uint32_t neighborIndex = 0; neighborIndex < RuleTileNeighborCount; ++neighborIndex)
            {
                const RuleTileNeighborCondition condition = neighborConditions[neighborIndex];
                if (condition == RuleTileNeighborCondition::Ignore)
                    continue;

                const uint16_t neighborTileIdentifier = neighborTileIdentifiers[neighborIndex];
                const bool isThisRuleTile = neighborTileIdentifier == ruleTileIdentifier;

                if (condition == RuleTileNeighborCondition::ThisRuleTile && !isThisRuleTile)
                    return false;

                if (condition == RuleTileNeighborCondition::NotThisRuleTile && isThisRuleTile)
                    return false;
            }

            return true;
        }

        void RotateNeighborConditionsClockwise(const RuleTileNeighborCondition source[RuleTileNeighborCount],
                                               uint32_t quarterTurnsClockwise,
                                               RuleTileNeighborCondition destination[RuleTileNeighborCount])
        {
            const uint32_t turns = quarterTurnsClockwise % 4u;
            for (uint32_t neighborIndex = 0; neighborIndex < RuleTileNeighborCount; ++neighborIndex)
            {
                const uint32_t rotatedIndex = (neighborIndex + turns * 2u) % RuleTileNeighborCount;
                destination[rotatedIndex] = source[neighborIndex];
            }
        }

        void MirrorNeighborConditionsHorizontal(const RuleTileNeighborCondition source[RuleTileNeighborCount],
                                                RuleTileNeighborCondition destination[RuleTileNeighborCount])
        {
            destination[0] = source[0];
            destination[1] = source[7];
            destination[2] = source[6];
            destination[3] = source[5];
            destination[4] = source[4];
            destination[5] = source[3];
            destination[6] = source[2];
            destination[7] = source[1];
        }

        void MirrorNeighborConditionsVertical(const RuleTileNeighborCondition source[RuleTileNeighborCount],
                                              RuleTileNeighborCondition destination[RuleTileNeighborCount])
        {
            destination[0] = source[4];
            destination[1] = source[3];
            destination[2] = source[2];
            destination[3] = source[1];
            destination[4] = source[0];
            destination[5] = source[7];
            destination[6] = source[6];
            destination[7] = source[5];
        }

        struct RuleTileMatchTransformResult
        {
            bool Matched = false;
            uint32_t QuarterTurnsClockwise = 0;
            bool MirrorHorizontal = false;
            bool MirrorVertical = false;
        };

        RuleTileMatchTransformResult TryMatchRule(const RuleTileRule &rule,
                                                  const uint16_t neighborTileIdentifiers[RuleTileNeighborCount],
                                                  uint16_t ruleTileIdentifier)
        {
            RuleTileMatchTransformResult result;
            RuleTileNeighborCondition transformedConditions[RuleTileNeighborCount] = {};

            auto tryConditions = [&](const RuleTileNeighborCondition conditions[RuleTileNeighborCount],
                                     uint32_t quarterTurnsClockwise,
                                     bool mirrorHorizontal,
                                     bool mirrorVertical) -> bool
            {
                if (!NeighborConditionsMatch(conditions, neighborTileIdentifiers, ruleTileIdentifier))
                    return false;

                result.Matched = true;
                result.QuarterTurnsClockwise = quarterTurnsClockwise;
                result.MirrorHorizontal = mirrorHorizontal;
                result.MirrorVertical = mirrorVertical;
                return true;
            };

            switch (rule.MatchTransform)
            {
                case RuleTileMatchTransform::Fixed:
                    tryConditions(rule.NeighborConditions, 0, false, false);
                    break;

                case RuleTileMatchTransform::Rotated:
                    for (uint32_t quarterTurnsClockwise = 0; quarterTurnsClockwise < 4; ++quarterTurnsClockwise)
                    {
                        RotateNeighborConditionsClockwise(
                                rule.NeighborConditions, quarterTurnsClockwise, transformedConditions);
                        if (tryConditions(transformedConditions, quarterTurnsClockwise, false, false))
                            break;
                    }
                    break;

                case RuleTileMatchTransform::MirrorHorizontal:
                    if (!tryConditions(rule.NeighborConditions, 0, false, false))
                    {
                        MirrorNeighborConditionsHorizontal(rule.NeighborConditions, transformedConditions);
                        tryConditions(transformedConditions, 0, true, false);
                    }
                    break;

                case RuleTileMatchTransform::MirrorVertical:
                    if (!tryConditions(rule.NeighborConditions, 0, false, false))
                    {
                        MirrorNeighborConditionsVertical(rule.NeighborConditions, transformedConditions);
                        tryConditions(transformedConditions, 0, false, true);
                    }
                    break;

                case RuleTileMatchTransform::MirrorBoth:
                    if (tryConditions(rule.NeighborConditions, 0, false, false))
                        break;

                    MirrorNeighborConditionsHorizontal(rule.NeighborConditions, transformedConditions);
                    if (tryConditions(transformedConditions, 0, true, false))
                        break;

                    MirrorNeighborConditionsVertical(rule.NeighborConditions, transformedConditions);
                    if (tryConditions(transformedConditions, 0, false, true))
                        break;

                    {
                        RuleTileNeighborCondition horizontalThenVertical[RuleTileNeighborCount] = {};
                        MirrorNeighborConditionsHorizontal(rule.NeighborConditions, transformedConditions);
                        MirrorNeighborConditionsVertical(transformedConditions, horizontalThenVertical);
                        tryConditions(horizontalThenVertical, 0, true, true);
                    }
                    break;
            }

            return result;
        }

        RuleTileResolveResult MakeResolveResult(uint16_t outputTileIdentifier,
                                                uint32_t quarterTurnsClockwise,
                                                bool mirrorHorizontal,
                                                bool mirrorVertical)
        {
            RuleTileResolveResult result;
            if (outputTileIdentifier == 0)
                return result;

            result.OutputTileIdentifier = outputTileIdentifier;
            result.QuarterTurnsClockwise = quarterTurnsClockwise;
            result.MirrorHorizontal = mirrorHorizontal;
            result.MirrorVertical = mirrorVertical;
            result.HasOutput = true;
            return result;
        }
    }

    RuleTileResolveResult RuleTileResolver::Resolve(const TileMapData &mapData,
                                                    const TileSet &tileSet,
                                                    int32_t tileX,
                                                    int32_t tileY,
                                                    uint16_t ruleTileIdentifier)
    {
        const RuleTileDefinition *ruleTileDefinition = tileSet.GetRuleTileDefinition(ruleTileIdentifier);
        if (!ruleTileDefinition)
            return {};

        uint16_t neighborTileIdentifiers[RuleTileNeighborCount] = {};
        for (uint32_t neighborIndex = 0; neighborIndex < RuleTileNeighborCount; ++neighborIndex)
        {
            neighborTileIdentifiers[neighborIndex] = mapData.GetTile(
                    tileX + NeighborOffsets[neighborIndex].x,
                    tileY + NeighborOffsets[neighborIndex].y);
        }

        for (uint32_t ruleIndex = 0; ruleIndex < static_cast<uint32_t>(ruleTileDefinition->Rules.size());
             ++ruleIndex)
        {
            const RuleTileRule &rule = ruleTileDefinition->Rules[ruleIndex];
            if (rule.OutputTileIdentifiers.empty())
                continue;

            const RuleTileMatchTransformResult match =
                    TryMatchRule(rule, neighborTileIdentifiers, ruleTileIdentifier);
            if (!match.Matched)
                continue;

            const uint16_t outputTileIdentifier = PickOutputTileIdentifier(
                    tileSet, rule.OutputTileIdentifiers, tileX, tileY, ruleIndex);
            if (outputTileIdentifier != 0)
            {
                return MakeResolveResult(outputTileIdentifier,
                                         match.QuarterTurnsClockwise,
                                         match.MirrorHorizontal,
                                         match.MirrorVertical);
            }

            break;
        }

        const uint16_t defaultOutputTileIdentifier = PickOutputTileIdentifier(
                tileSet,
                ruleTileDefinition->DefaultOutputTileIdentifiers,
                tileX,
                tileY,
                DefaultOutputRuleIndexSentinel);
        return MakeResolveResult(defaultOutputTileIdentifier, 0, false, false);
    }

    void RuleTileResolver::ApplyTextureCoordinateTransform(glm::vec2 textureCoordinates[4],
                                                           uint32_t quarterTurnsClockwise,
                                                           bool mirrorHorizontal,
                                                           bool mirrorVertical)
    {
        glm::vec2 originalTextureCoordinates[4] = {
                textureCoordinates[0],
                textureCoordinates[1],
                textureCoordinates[2],
                textureCoordinates[3]};

        const uint32_t turns = quarterTurnsClockwise % 4u;
        for (uint32_t vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
            textureCoordinates[vertexIndex] = originalTextureCoordinates[(vertexIndex + turns) % 4];

        if (mirrorHorizontal)
        {
            std::swap(textureCoordinates[0], textureCoordinates[1]);
            std::swap(textureCoordinates[3], textureCoordinates[2]);
        }

        if (mirrorVertical)
        {
            std::swap(textureCoordinates[0], textureCoordinates[3]);
            std::swap(textureCoordinates[1], textureCoordinates[2]);
        }
    }

} // namespace Himii
