#pragma once

#include "Resource/Asset.h"
#include "Module/Tilemap/TileMapChunk.h"

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace Himii
{

    // TileMapData 资源：稀疏分块存储，有符号格子坐标，以实体为原点可无限扩展
    class TileMapData : public Asset {
    public:
        TileMapData() = default;
        virtual ~TileMapData() = default;

        virtual AssetType GetType() const override
        {
            return AssetType::TileMap;
        }

        void SetTileSetHandle(AssetHandle handle) { m_TileSetHandle = handle; }
        AssetHandle GetTileSetHandle() const { return m_TileSetHandle; }

        float GetCellSize() const { return m_CellSize; }
        void SetCellSize(float size) { m_CellSize = size; }

        /** @deprecated 无瓦片时返回 0；有瓦片时为 bounds 宽度 (maxX - minX + 1) */
        uint32_t GetWidth() const;
        /** @deprecated 无瓦片时返回 0；有瓦片时为 bounds 高度 (maxY - minY + 1) */
        uint32_t GetHeight() const;

        bool HasBounds() const { return m_HasBounds; }
        void GetBounds(int32_t& minTileX, int32_t& minTileY, int32_t& maxTileX, int32_t& maxTileY) const;
        void Clear();

        void SetTile(int32_t tileX, int32_t tileY, uint16_t tileIdentifier);
        uint16_t GetTile(int32_t tileX, int32_t tileY) const;

        template<typename Callback>
        void ForEachTile(Callback&& callback) const
        {
            for (const auto& [chunkKey, chunk] : m_Chunks)
            {
                for (int32_t localY = 0; localY < TileMapChunkSize; ++localY)
                {
                    for (int32_t localX = 0; localX < TileMapChunkSize; ++localX)
                    {
                        const size_t index =
                                static_cast<size_t>(localX)
                                + static_cast<size_t>(localY) * TileMapChunkSize;
                        const uint16_t tileIdentifier = chunk.Tiles[index];
                        if (tileIdentifier == 0)
                            continue;

                        const int32_t tileX =
                                chunkKey.ChunkX * TileMapChunkSize + localX;
                        const int32_t tileY =
                                chunkKey.ChunkY * TileMapChunkSize + localY;
                        callback(tileX, tileY, tileIdentifier);
                    }
                }
            }
        }

        const std::unordered_map<TileMapChunkKey, TileMapChunk, TileMapChunkKeyHash>& GetChunks() const
        {
            return m_Chunks;
        }

        std::unordered_map<TileMapChunkKey, TileMapChunk, TileMapChunkKeyHash>& GetChunks()
        {
            return m_Chunks;
        }

        void ImportLegacyDenseTiles(uint32_t halfWidth, uint32_t halfHeight,
                                    const std::vector<uint16_t>& denseTiles);

    private:
        TileMapChunk& GetOrCreateChunk(const TileMapChunkKey& chunkKey);
        const TileMapChunk* FindChunk(const TileMapChunkKey& chunkKey) const;
        void UpdateBoundsOnSet(int32_t tileX, int32_t tileY);
        void RecomputeBounds();
        bool IsChunkEmpty(const TileMapChunk& chunk) const;
        void RemoveChunkIfEmpty(const TileMapChunkKey& chunkKey);

        AssetHandle m_TileSetHandle = 0;
        float m_CellSize = 1.0f;

        std::unordered_map<TileMapChunkKey, TileMapChunk, TileMapChunkKeyHash> m_Chunks;

        bool m_HasBounds = false;
        int32_t m_MinTileX = 0;
        int32_t m_MinTileY = 0;
        int32_t m_MaxTileX = 0;
        int32_t m_MaxTileY = 0;
    };

} // namespace Himii
