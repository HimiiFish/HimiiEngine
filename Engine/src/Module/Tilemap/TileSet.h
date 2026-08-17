#pragma once

#include "Resource/Asset.h"
#include "Module/Render/RenderCore/Texture.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Himii
{

    constexpr uint32_t RuleTileNeighborCount = 8;

    // Tile 来源类型
    enum class TileSourceType : uint8_t
    {
        Atlas = 0,      // 从图集纹理中切分
        Individual = 1  // 独立纹理
    };

    enum class RuleTileNeighborCondition : uint8_t
    {
        Ignore = 0,
        ThisRuleTile = 1,
        NotThisRuleTile = 2
    };

    enum class RuleTileMatchTransform : uint8_t
    {
        Fixed = 0,
        Rotated = 1,
        MirrorHorizontal = 2,
        MirrorVertical = 3,
        MirrorBoth = 4
    };

    // 图集来源：一张图集纹理 + 网格切分信息
    struct TileAtlasSource
    {
        AssetHandle TextureHandle = 0;  // 引用图集纹理
        uint32_t TileSize = 16;         // 每格像素尺寸（正方形）

        // 运行时缓存（不序列化）
        Ref<Texture2D> CachedTexture;
    };

    // 单个 Tile 的定义
    struct TileDef
    {
        uint16_t ID = 0;

        TileSourceType SourceType = TileSourceType::Atlas;

        // Atlas 模式
        uint32_t AtlasSourceIndex = 0;  // 对应 TileSet::AtlasSources 的索引
        glm::ivec2 AtlasCoords{0, 0};  // 在图集网格中的位置 (列, 行)

        // Individual 模式
        AssetHandle IndividualTextureHandle = 0;

        // 通用属性
        glm::vec4 Tint{1.0f, 1.0f, 1.0f, 1.0f};
        bool Collidable = false;        // 后续扩展：碰撞

        // 运行时缓存（不序列化）
        Ref<Texture2D> CachedIndividualTexture;
    };

    struct RuleTileRule
    {
        RuleTileNeighborCondition NeighborConditions[RuleTileNeighborCount] = {};
        RuleTileMatchTransform MatchTransform = RuleTileMatchTransform::Fixed;
        std::vector<uint16_t> OutputTileIdentifiers;
    };

    struct RuleTileDefinition
    {
        uint16_t Identifier = 0;
        std::string DisplayName;
        bool Collidable = false;
        std::vector<uint16_t> DefaultOutputTileIdentifiers;
        std::vector<RuleTileRule> Rules;
    };

    // TileSet 资源：管理一套 Tile 定义
    class TileSet : public Asset {
    public:
        TileSet() = default;
        virtual ~TileSet() = default;

        virtual AssetType GetType() const override
        {
            return AssetType::TileSet;
        }

        // --- Atlas Sources ---
        void AddAtlasSource(const TileAtlasSource &source)
        {
            m_AtlasSources.push_back(source);
        }

        const std::vector<TileAtlasSource> &GetAtlasSources() const { return m_AtlasSources; }
        std::vector<TileAtlasSource> &GetAtlasSources() { return m_AtlasSources; }

        // --- Tile Definitions ---
        void AddTileDef(const TileDef &tileDef)
        {
            if (tileDef.ID == 0 || GetRuleTileDefinition(tileDef.ID) != nullptr)
                return;

            m_TileDefs[tileDef.ID] = tileDef;
        }

        const TileDef *GetTileDef(uint16_t id) const
        {
            auto it = m_TileDefs.find(id);
            return it != m_TileDefs.end() ? &it->second : nullptr;
        }

        const std::unordered_map<uint16_t, TileDef> &GetTileDefs() const { return m_TileDefs; }
        std::unordered_map<uint16_t, TileDef> &GetTileDefs() { return m_TileDefs; }

        void AddRuleTileDefinition(const RuleTileDefinition &definition)
        {
            if (definition.Identifier == 0 || GetTileDef(definition.Identifier) != nullptr)
                return;

            m_RuleTileDefinitions[definition.Identifier] = definition;
        }

        void RemoveRuleTileDefinition(uint16_t identifier)
        {
            m_RuleTileDefinitions.erase(identifier);
        }

        const RuleTileDefinition *GetRuleTileDefinition(uint16_t identifier) const
        {
            auto iterator = m_RuleTileDefinitions.find(identifier);
            return iterator != m_RuleTileDefinitions.end() ? &iterator->second : nullptr;
        }

        RuleTileDefinition *GetRuleTileDefinition(uint16_t identifier)
        {
            auto iterator = m_RuleTileDefinitions.find(identifier);
            return iterator != m_RuleTileDefinitions.end() ? &iterator->second : nullptr;
        }

        const std::unordered_map<uint16_t, RuleTileDefinition> &GetRuleTileDefinitions() const
        {
            return m_RuleTileDefinitions;
        }

        std::unordered_map<uint16_t, RuleTileDefinition> &GetRuleTileDefinitions()
        {
            return m_RuleTileDefinitions;
        }

        bool IsTileIdentifierOccupied(uint16_t identifier) const
        {
            return GetTileDef(identifier) != nullptr || GetRuleTileDefinition(identifier) != nullptr;
        }

        bool HasPaintableRuleTile(uint16_t identifier) const
        {
            const RuleTileDefinition *definition = GetRuleTileDefinition(identifier);
            if (!definition)
                return false;

            for (uint16_t outputTileIdentifier : definition->DefaultOutputTileIdentifiers)
            {
                if (GetTileDef(outputTileIdentifier) != nullptr)
                    return true;
            }

            return false;
        }

        uint16_t GetNextTileIdentifier() const
        {
            uint16_t maximumIdentifier = 0;
            for (const auto &[tileIdentifier, tileDefinition] : m_TileDefs)
            {
                if (tileIdentifier > maximumIdentifier)
                    maximumIdentifier = tileIdentifier;
            }
            for (const auto &[ruleTileIdentifier, ruleTileDefinition] : m_RuleTileDefinitions)
            {
                if (ruleTileIdentifier > maximumIdentifier)
                    maximumIdentifier = ruleTileIdentifier;
            }

            if (maximumIdentifier == 0xFFFFu)
                return 0;

            return static_cast<uint16_t>(maximumIdentifier + 1);
        }

        uint16_t GetNextTileID() const
        {
            return GetNextTileIdentifier();
        }

        void ClearTileDefs()
        {
            m_TileDefs.clear();
        }

        void GenerateGridTileDefs(uint32_t atlasSourceIndex,
                                  uint32_t columnCount,
                                  uint32_t rowCount,
                                  const std::unordered_map<uint16_t, TileDef>* previousDefinitions = nullptr)
        {
            std::unordered_map<uint64_t, const TileDef*> collidableByAtlasCell;
            if (previousDefinitions)
            {
                for (const auto& [previousIdentifier, previousDefinition] : *previousDefinitions)
                {
                    if (previousDefinition.SourceType != TileSourceType::Atlas)
                        continue;

                    const uint64_t atlasKey =
                            (static_cast<uint64_t>(previousDefinition.AtlasSourceIndex) << 32)
                            | (static_cast<uint32_t>(previousDefinition.AtlasCoords.x) << 16)
                            | static_cast<uint32_t>(previousDefinition.AtlasCoords.y);
                    collidableByAtlasCell[atlasKey] = &previousDefinition;
                }
            }

            ClearTileDefs();
            uint16_t tileIdentifier = 1;
            for (uint32_t row = 0; row < rowCount; ++row)
            {
                for (uint32_t column = 0; column < columnCount; ++column)
                {
                    while (GetRuleTileDefinition(tileIdentifier) != nullptr)
                    {
                        if (tileIdentifier == 0xFFFFu)
                            return;
                        tileIdentifier++;
                    }

                    TileDef tileDefinition;
                    tileDefinition.ID = tileIdentifier;
                    tileDefinition.SourceType = TileSourceType::Atlas;
                    tileDefinition.AtlasSourceIndex = atlasSourceIndex;
                    tileDefinition.AtlasCoords = {static_cast<int>(column), static_cast<int>(row)};

                    const uint64_t atlasKey =
                            (static_cast<uint64_t>(atlasSourceIndex) << 32)
                            | (static_cast<uint32_t>(column) << 16)
                            | static_cast<uint32_t>(row);
                    const auto preservedIterator = collidableByAtlasCell.find(atlasKey);
                    if (preservedIterator != collidableByAtlasCell.end())
                    {
                        tileDefinition.Collidable = preservedIterator->second->Collidable;
                        tileDefinition.Tint = preservedIterator->second->Tint;
                    }

                    AddTileDef(tileDefinition);

                    if (tileIdentifier == 0xFFFFu)
                        return;
                    tileIdentifier++;
                }
            }
        }

    private:
        std::vector<TileAtlasSource> m_AtlasSources;
        std::unordered_map<uint16_t, TileDef> m_TileDefs;
        std::unordered_map<uint16_t, RuleTileDefinition> m_RuleTileDefinitions;
    };

} // namespace Himii
