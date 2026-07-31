#include "Hepch.h"
#include "Resource/PackFile.h"

#include "EngineCore/Core/Log.h"

#include <algorithm>

namespace Himii
{
    std::string PackFileReader::NormalizeRelativePath(std::string relativePath)
    {
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
        while (!relativePath.empty() && relativePath.front() == '/')
            relativePath.erase(relativePath.begin());
        return relativePath;
    }

    bool PackFileReader::Open(const std::filesystem::path &packFilePath)
    {
        Close();

        std::ifstream inputStream(packFilePath, std::ios::binary);
        if (!inputStream)
        {
            HIMII_CORE_ERROR("PackFileReader: failed to open '{0}'", packFilePath.string());
            return false;
        }

        inputStream.read(reinterpret_cast<char *>(&m_Header), sizeof(PackFileHeader));
        if (!inputStream)
        {
            HIMII_CORE_ERROR("PackFileReader: failed to read header from '{0}'", packFilePath.string());
            return false;
        }

        if (std::memcmp(m_Header.Magic, kPackFileMagic, 4) != 0)
        {
            HIMII_CORE_ERROR("PackFileReader: invalid magic in '{0}'", packFilePath.string());
            return false;
        }

        if (m_Header.Version != kPackFileVersion)
        {
            HIMII_CORE_ERROR("PackFileReader: unsupported version {0} in '{1}'", m_Header.Version, packFilePath.string());
            return false;
        }

        inputStream.seekg(static_cast<std::streamoff>(m_Header.IndexOffset), std::ios::beg);
        for (uint32_t entryIndex = 0; entryIndex < m_Header.EntryCount; ++entryIndex)
        {
            uint16_t pathLength = 0;
            inputStream.read(reinterpret_cast<char *>(&pathLength), sizeof(uint16_t));
            if (!inputStream || pathLength == 0)
            {
                HIMII_CORE_ERROR("PackFileReader: corrupt index entry in '{0}'", packFilePath.string());
                return false;
            }

            std::string relativePath(pathLength, '\0');
            inputStream.read(relativePath.data(), pathLength);

            PackEntry entry{};
            inputStream.read(reinterpret_cast<char *>(&entry.DataOffset), sizeof(uint64_t));
            inputStream.read(reinterpret_cast<char *>(&entry.DataSize), sizeof(uint64_t));
            inputStream.read(reinterpret_cast<char *>(&entry.Flags), sizeof(uint32_t));
            if (!inputStream)
            {
                HIMII_CORE_ERROR("PackFileReader: corrupt index payload in '{0}'", packFilePath.string());
                return false;
            }

            m_Entries[NormalizeRelativePath(std::move(relativePath))] = entry;
        }

        m_PackFilePath = packFilePath;
        m_DataSectionBase = m_Header.DataOffset;
        m_IsOpen = true;
        HIMII_CORE_INFO("PackFileReader: loaded {0} entries from '{1}'", m_Entries.size(), packFilePath.string());
        return true;
    }

    void PackFileReader::Close()
    {
        m_IsOpen = false;
        m_PackFilePath.clear();
        m_Header = {};
        m_DataSectionBase = 0;
        m_Entries.clear();
    }

    bool PackFileReader::HasEntry(const std::string &relativePath) const
    {
        return m_Entries.find(NormalizeRelativePath(relativePath)) != m_Entries.end();
    }

    bool PackFileReader::ReadEntry(const std::string &relativePath, std::vector<uint8_t> &outBytes) const
    {
        outBytes.clear();
        if (!m_IsOpen)
            return false;

        const std::string normalizedPath = NormalizeRelativePath(relativePath);
        const auto iterator = m_Entries.find(normalizedPath);
        if (iterator == m_Entries.end())
            return false;

        if ((iterator->second.Flags & kPackEntryFlagCompressed) != 0)
        {
            HIMII_CORE_ERROR("PackFileReader: compressed entries are not supported yet ('{0}')", normalizedPath);
            return false;
        }

        std::ifstream inputStream(m_PackFilePath, std::ios::binary);
        if (!inputStream)
            return false;

        inputStream.seekg(static_cast<std::streamoff>(m_DataSectionBase + iterator->second.DataOffset), std::ios::beg);
        outBytes.resize(static_cast<size_t>(iterator->second.DataSize));
        inputStream.read(reinterpret_cast<char *>(outBytes.data()), static_cast<std::streamsize>(iterator->second.DataSize));
        return static_cast<bool>(inputStream);
    }

    bool PackFileWriter::Write(const std::filesystem::path &outputPath, const std::vector<PackInputEntry> &entries)
    {
        if (entries.empty())
            return false;

        std::vector<PackInputEntry> sortedEntries = entries;
        for (PackInputEntry &entry : sortedEntries)
            entry.RelativePath = PackFileReader::NormalizeRelativePath(std::move(entry.RelativePath));

        std::sort(sortedEntries.begin(), sortedEntries.end(),
                  [](const PackInputEntry &left, const PackInputEntry &right)
                  {
                      return left.RelativePath < right.RelativePath;
                  });

        const std::filesystem::path parentDirectory = outputPath.parent_path();
        if (!parentDirectory.empty())
            std::filesystem::create_directories(parentDirectory);

        std::ofstream outputStream(outputPath, std::ios::binary | std::ios::trunc);
        if (!outputStream)
            return false;

        PackFileHeader header{};
        std::memcpy(header.Magic, kPackFileMagic, 4);
        header.Version = kPackFileVersion;
        header.EntryCount = static_cast<uint32_t>(sortedEntries.size());
        header.IndexOffset = sizeof(PackFileHeader);

        header.DataOffset = header.IndexOffset;
        for (const PackInputEntry &entry : sortedEntries)
        {
            header.DataOffset += sizeof(uint16_t) + entry.RelativePath.size() + sizeof(uint64_t) + sizeof(uint64_t) +
                                 sizeof(uint32_t);
        }

        outputStream.write(reinterpret_cast<const char *>(&header), sizeof(PackFileHeader));

        uint64_t runningDataOffset = 0;
        for (const PackInputEntry &entry : sortedEntries)
        {
            const uint16_t pathLength = static_cast<uint16_t>(entry.RelativePath.size());
            const uint64_t dataSize = entry.Bytes.size();
            const uint32_t flags = 0;

            outputStream.write(reinterpret_cast<const char *>(&pathLength), sizeof(uint16_t));
            outputStream.write(entry.RelativePath.data(), static_cast<std::streamsize>(entry.RelativePath.size()));
            outputStream.write(reinterpret_cast<const char *>(&runningDataOffset), sizeof(uint64_t));
            outputStream.write(reinterpret_cast<const char *>(&dataSize), sizeof(uint64_t));
            outputStream.write(reinterpret_cast<const char *>(&flags), sizeof(uint32_t));

            runningDataOffset += dataSize;
        }

        for (const PackInputEntry &entry : sortedEntries)
        {
            if (!entry.Bytes.empty())
                outputStream.write(reinterpret_cast<const char *>(entry.Bytes.data()),
                                   static_cast<std::streamsize>(entry.Bytes.size()));
        }

        return static_cast<bool>(outputStream);
    }
}
