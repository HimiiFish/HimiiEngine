#include "Hepch.h"
#include "Module/Tilemap/TileMapData.h"

namespace Himii
{

    uint32_t TileMapData::GetWidth() const
    {
        if (!m_HasBounds)
            return 0;
        return static_cast<uint32_t>(m_MaxTileX - m_MinTileX + 1);
    }

    uint32_t TileMapData::GetHeight() const
    {
        if (!m_HasBounds)
            return 0;
        return static_cast<uint32_t>(m_MaxTileY - m_MinTileY + 1);
    }

    void TileMapData::GetBounds(int32_t& minTileX, int32_t& minTileY, int32_t& maxTileX,
                                int32_t& maxTileY) const
    {
        if (!m_HasBounds)
        {
            minTileX = minTileY = maxTileX = maxTileY = 0;
            return;
        }

        minTileX = m_MinTileX;
        minTileY = m_MinTileY;
        maxTileX = m_MaxTileX;
        maxTileY = m_MaxTileY;
    }

    void TileMapData::Clear()
    {
        m_Chunks.clear();
        m_HasBounds = false;
        m_MinTileX = m_MinTileY = m_MaxTileX = m_MaxTileY = 0;
    }

    TileMapChunk& TileMapData::GetOrCreateChunk(const TileMapChunkKey& chunkKey)
    {
        return m_Chunks[chunkKey];
    }

    const TileMapChunk* TileMapData::FindChunk(const TileMapChunkKey& chunkKey) const
    {
        const auto iterator = m_Chunks.find(chunkKey);
        if (iterator == m_Chunks.end())
            return nullptr;
        return &iterator->second;
    }

    bool TileMapData::IsChunkEmpty(const TileMapChunk& chunk) const
    {
        for (uint16_t tileIdentifier : chunk.Tiles)
        {
            if (tileIdentifier != 0)
                return false;
        }
        return true;
    }

    void TileMapData::RemoveChunkIfEmpty(const TileMapChunkKey& chunkKey)
    {
        const auto iterator = m_Chunks.find(chunkKey);
        if (iterator == m_Chunks.end())
            return;

        if (IsChunkEmpty(iterator->second))
            m_Chunks.erase(iterator);
    }

    void TileMapData::UpdateBoundsOnSet(int32_t tileX, int32_t tileY)
    {
        if (!m_HasBounds)
        {
            m_HasBounds = true;
            m_MinTileX = m_MaxTileX = tileX;
            m_MinTileY = m_MaxTileY = tileY;
            return;
        }

        m_MinTileX = std::min(m_MinTileX, tileX);
        m_MinTileY = std::min(m_MinTileY, tileY);
        m_MaxTileX = std::max(m_MaxTileX, tileX);
        m_MaxTileY = std::max(m_MaxTileY, tileY);
    }

    void TileMapData::RecomputeBounds()
    {
        m_HasBounds = false;
        m_MinTileX = m_MinTileY = m_MaxTileX = m_MaxTileY = 0;

        ForEachTile([&](int32_t tileX, int32_t tileY, uint16_t)
        {
            UpdateBoundsOnSet(tileX, tileY);
        });
    }

    void TileMapData::SetTile(int32_t tileX, int32_t tileY, uint16_t tileIdentifier)
    {
        const TileMapChunkKey chunkKey = TileMapChunkKeyFromTile(tileX, tileY);
        int32_t localX = 0;
        int32_t localY = 0;
        TileMapChunkLocalIndex(tileX, tileY, localX, localY);

        if (tileIdentifier == 0)
        {
            const TileMapChunk* chunk = FindChunk(chunkKey);
            if (!chunk)
                return;

            const size_t index =
                    static_cast<size_t>(localX)
                    + static_cast<size_t>(localY) * TileMapChunkSize;

            auto iterator = m_Chunks.find(chunkKey);
            if (iterator == m_Chunks.end())
                return;

            iterator->second.Tiles[index] = 0;
            RemoveChunkIfEmpty(chunkKey);

            if (m_HasBounds
                && (tileX == m_MinTileX || tileX == m_MaxTileX
                    || tileY == m_MinTileY || tileY == m_MaxTileY))
                RecomputeBounds();

            return;
        }

        TileMapChunk& chunk = GetOrCreateChunk(chunkKey);
        const size_t index =
                static_cast<size_t>(localX)
                + static_cast<size_t>(localY) * TileMapChunkSize;
        chunk.Tiles[index] = tileIdentifier;
        UpdateBoundsOnSet(tileX, tileY);
    }

    uint16_t TileMapData::GetTile(int32_t tileX, int32_t tileY) const
    {
        const TileMapChunkKey chunkKey = TileMapChunkKeyFromTile(tileX, tileY);
        const TileMapChunk* chunk = FindChunk(chunkKey);
        if (!chunk)
            return 0;

        int32_t localX = 0;
        int32_t localY = 0;
        TileMapChunkLocalIndex(tileX, tileY, localX, localY);

        const size_t index =
                static_cast<size_t>(localX)
                + static_cast<size_t>(localY) * TileMapChunkSize;
        return chunk->Tiles[index];
    }

    void TileMapData::ImportLegacyDenseTiles(uint32_t halfWidth, uint32_t halfHeight,
                                             const std::vector<uint16_t>& denseTiles)
    {
        Clear();

        const uint32_t width = 2 * halfWidth + 1;
        const uint32_t height = 2 * halfHeight + 1;
        const size_t expectedSize = static_cast<size_t>(width) * height;

        if (denseTiles.size() < expectedSize)
            return;

        for (uint32_t oldY = 0; oldY < height; ++oldY)
        {
            for (uint32_t oldX = 0; oldX < width; ++oldX)
            {
                const uint16_t tileIdentifier = denseTiles[oldX + oldY * width];
                if (tileIdentifier == 0)
                    continue;

                const int32_t signedX = static_cast<int32_t>(oldX) - static_cast<int32_t>(halfWidth);
                const int32_t signedY = static_cast<int32_t>(oldY) - static_cast<int32_t>(halfHeight);
                SetTile(signedX, signedY, tileIdentifier);
            }
        }
    }

} // namespace Himii
