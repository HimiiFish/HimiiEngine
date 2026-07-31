#pragma once

#include "Resource/PackFileFormat.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Himii
{
    class PackFileReader
    {
    public:
        bool Open(const std::filesystem::path &packFilePath);
        void Close();

        bool IsOpen() const { return m_IsOpen; }
        bool HasEntry(const std::string &relativePath) const;
        bool ReadEntry(const std::string &relativePath, std::vector<uint8_t> &outBytes) const;

        static std::string NormalizeRelativePath(std::string relativePath);

    private:
        struct PackEntry
        {
            uint64_t DataOffset = 0;
            uint64_t DataSize = 0;
            uint32_t Flags = 0;
        };

        bool m_IsOpen = false;
        std::filesystem::path m_PackFilePath;
        PackFileHeader m_Header{};
        uint64_t m_DataSectionBase = 0;
        std::unordered_map<std::string, PackEntry> m_Entries;
    };

    class PackFileWriter
    {
    public:
        struct PackInputEntry
        {
            std::string RelativePath;
            std::vector<uint8_t> Bytes;
        };

        static bool Write(const std::filesystem::path &outputPath, const std::vector<PackInputEntry> &entries);
    };
}
