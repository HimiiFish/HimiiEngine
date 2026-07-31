#pragma once

#include <cstdint>

namespace Himii
{
    inline constexpr char kPackFileMagic[4] = {'H', 'P', 'K', '1'};
    inline constexpr uint32_t kPackFileVersion = 1;

#pragma pack(push, 1)
    struct PackFileHeader
    {
        char Magic[4];
        uint32_t Version;
        uint32_t EntryCount;
        uint64_t IndexOffset;
        uint64_t DataOffset;
    };

    struct PackFileIndexEntry
    {
        uint16_t PathLength;
        // Followed by PathLength bytes of UTF-8 path, then:
        // uint64_t DataOffset, uint64_t DataSize, uint32_t Flags
    };
#pragma pack(pop)

    inline constexpr uint32_t kPackEntryFlagCompressed = 1u << 0;
}
