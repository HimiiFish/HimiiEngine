#include "Hepch.h"
#include "EngineCore/Core/FileSystem.h"

#include "Resource/PackFile.h"
#include "EngineCore/Core/Log.h"

#include <fstream>

namespace Himii
{
    namespace
    {
        std::filesystem::path s_EngineContentRoot;
        std::filesystem::path s_ExecutableDirectory;
        bool s_PreferLooseFiles = true;
        PackFileReader s_PackFileReader;
    }

    void FileSystem::Init(const std::filesystem::path &engineContentRoot,
                          const std::filesystem::path &executableDirectory, bool preferLooseFiles)
    {
        Shutdown();

        s_EngineContentRoot = engineContentRoot;
        s_ExecutableDirectory = executableDirectory;
        s_PreferLooseFiles = preferLooseFiles;

        const std::filesystem::path packFilePath = s_EngineContentRoot / "engine.hpck";
        if (std::filesystem::exists(packFilePath))
        {
            if (!s_PackFileReader.Open(packFilePath))
                HIMII_CORE_WARNING("FileSystem: failed to open pack file '{0}'", packFilePath.string());
        }
        else if (!s_PreferLooseFiles)
        {
            HIMII_CORE_WARNING("FileSystem: pack file not found at '{0}'", packFilePath.string());
        }
    }

    void FileSystem::Shutdown()
    {
        s_PackFileReader.Close();
        s_EngineContentRoot.clear();
        s_ExecutableDirectory.clear();
        s_PreferLooseFiles = true;
    }

    const std::filesystem::path &FileSystem::GetEngineContentRoot()
    {
        return s_EngineContentRoot;
    }

    const std::filesystem::path &FileSystem::GetExecutableDirectory()
    {
        return s_ExecutableDirectory;
    }

    std::filesystem::path FileSystem::GetWritableCacheRoot()
    {
#if defined(HIMII_DEBUG)
        return s_EngineContentRoot / "assets" / "cache";
#else
        return s_ExecutableDirectory / "shader_cache";
#endif
    }

    std::string FileSystem::NormalizeRelativePath(std::string relativePath)
    {
        return PackFileReader::NormalizeRelativePath(std::move(relativePath));
    }

    std::optional<std::vector<uint8_t>> FileSystem::ReadLooseBytes(const std::string &relativePath)
    {
        const std::filesystem::path loosePath = s_EngineContentRoot / relativePath;
        if (!std::filesystem::exists(loosePath))
            return std::nullopt;

        std::ifstream inputStream(loosePath, std::ios::binary);
        if (!inputStream)
            return std::nullopt;

        inputStream.seekg(0, std::ios::end);
        const std::streamsize fileSize = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);

        std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
        inputStream.read(reinterpret_cast<char *>(bytes.data()), fileSize);
        if (!inputStream)
            return std::nullopt;

        return bytes;
    }

    bool FileSystem::Exists(const std::string &relativePath)
    {
        const std::string normalizedPath = NormalizeRelativePath(relativePath);

        if (s_PreferLooseFiles)
        {
            const std::filesystem::path loosePath = s_EngineContentRoot / normalizedPath;
            if (std::filesystem::exists(loosePath))
                return true;
        }

        return s_PackFileReader.IsOpen() && s_PackFileReader.HasEntry(normalizedPath);
    }

    std::optional<std::vector<uint8_t>> FileSystem::ReadBytes(const std::string &relativePath)
    {
        const std::string normalizedPath = NormalizeRelativePath(relativePath);

        if (s_PreferLooseFiles)
        {
            if (auto looseBytes = ReadLooseBytes(normalizedPath))
                return looseBytes;
        }

        if (s_PackFileReader.IsOpen())
        {
            std::vector<uint8_t> packedBytes;
            if (s_PackFileReader.ReadEntry(normalizedPath, packedBytes))
                return packedBytes;
        }

        if (!s_PreferLooseFiles)
            return ReadLooseBytes(normalizedPath);

        return std::nullopt;
    }

    std::string FileSystem::ReadText(const std::string &relativePath)
    {
        const auto bytes = ReadBytes(relativePath);
        if (!bytes)
        {
            HIMII_CORE_ERROR("FileSystem: could not read '{0}'", relativePath);
            return {};
        }

        return std::string(reinterpret_cast<const char *>(bytes->data()), bytes->size());
    }

    std::filesystem::path FileSystem::MaterializeLooseFile(const std::string &relativePath)
    {
        const std::string normalizedPath = NormalizeRelativePath(relativePath);
        const std::filesystem::path loosePath = s_EngineContentRoot / normalizedPath;
        if (std::filesystem::exists(loosePath))
            return loosePath;

        const auto bytes = ReadBytes(normalizedPath);
        if (!bytes)
            return loosePath;

        const std::filesystem::path tempDirectory = std::filesystem::temp_directory_path() / "HimiiEngine" / "materialized";
        std::filesystem::create_directories(tempDirectory);
        const std::filesystem::path tempPath = tempDirectory / std::filesystem::path(normalizedPath).filename();

        std::ofstream outputStream(tempPath, std::ios::binary | std::ios::trunc);
        outputStream.write(reinterpret_cast<const char *>(bytes->data()), static_cast<std::streamsize>(bytes->size()));
        return tempPath;
    }
}
