#pragma once

#include "Resource/Asset.h"
#include "Module/Render/RenderCore/Texture.h"

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace Himii
{

    // Tile 来源类型
    enum class TileSourceType : uint8_t
    {
        Atlas = 0,      // 从图集纹理中切分
        Individual = 1  // 独立纹理
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
            m_TileDefs[tileDef.ID] = tileDef;
        }

        const TileDef *GetTileDef(uint16_t id) const
        {
            auto it = m_TileDefs.find(id);
            return it != m_TileDefs.end() ? &it->second : nullptr;
        }

        const std::unordered_map<uint16_t, TileDef> &GetTileDefs() const { return m_TileDefs; }
        std::unordered_map<uint16_t, TileDef> &GetTileDefs() { return m_TileDefs; }

        // 获取下一个可用的 TileID
        uint16_t GetNextTileID() const
        {
            uint16_t maxID = 0;
            for (const auto &[id, def] : m_TileDefs)
            {
                if (id > maxID) maxID = id;
            }
            return maxID + 1;
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
                    TileDef tileDefinition;
                    tileDefinition.ID = tileIdentifier++;
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
                }
            }
        }

    private:
        std::vector<TileAtlasSource> m_AtlasSources;
        std::unordered_map<uint16_t, TileDef> m_TileDefs;
    };

} // namespace Himii
