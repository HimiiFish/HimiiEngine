#pragma once

#include <array>
#include <cstdint>

namespace Himii
{

    constexpr int32_t TileMapChunkSize = 16;
    constexpr size_t TileMapChunkTileCount =
        static_cast<size_t>(TileMapChunkSize) * static_cast<size_t>(TileMapChunkSize);

    inline int32_t FloorDiv(int32_t value, int32_t divisor)
    {
        if (divisor == 0)
            return 0;

        int32_t quotient = value / divisor;
        const int32_t remainder = value % divisor;
        if ((remainder != 0) && ((remainder < 0) != (divisor < 0)))
            --quotient;
        return quotient;
    }

    struct TileMapChunkKey
    {
        int32_t ChunkX = 0;
        int32_t ChunkY = 0;

        bool operator==(const TileMapChunkKey& other) const
        {
            return ChunkX == other.ChunkX && ChunkY == other.ChunkY;
        }
    };

    struct TileMapChunkKeyHash
    {
        size_t operator()(const TileMapChunkKey& key) const
        {
            const uint64_t packedX = static_cast<uint32_t>(key.ChunkX);
            const uint64_t packedY = static_cast<uint32_t>(key.ChunkY);
            return static_cast<size_t>(packedX ^ (packedY << 16));
        }
    };

    struct TileMapChunk
    {
        std::array<uint16_t, TileMapChunkTileCount> Tiles{};

        TileMapChunk()
        {
            Tiles.fill(0);
        }
    };

    inline TileMapChunkKey TileMapChunkKeyFromTile(int32_t tileX, int32_t tileY)
    {
        return {
            FloorDiv(tileX, TileMapChunkSize),
            FloorDiv(tileY, TileMapChunkSize)};
    }

    inline void TileMapChunkLocalIndex(int32_t tileX, int32_t tileY, int32_t& localX, int32_t& localY)
    {
        const int32_t chunkX = FloorDiv(tileX, TileMapChunkSize);
        const int32_t chunkY = FloorDiv(tileY, TileMapChunkSize);
        localX = tileX - chunkX * TileMapChunkSize;
        localY = tileY - chunkY * TileMapChunkSize;
    }

} // namespace Himii
