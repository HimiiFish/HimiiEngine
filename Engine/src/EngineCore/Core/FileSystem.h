#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Himii
{
    class FileSystem
    {
    public:
        static void Init(const std::filesystem::path &engineContentRoot, const std::filesystem::path &executableDirectory,
                         bool preferLooseFiles);
        static void Shutdown();

        static const std::filesystem::path &GetEngineContentRoot();
        static const std::filesystem::path &GetExecutableDirectory();
        static std::filesystem::path GetWritableCacheRoot();

        static bool Exists(const std::string &relativePath);
        static std::optional<std::vector<uint8_t>> ReadBytes(const std::string &relativePath);
        static std::string ReadText(const std::string &relativePath);

        /// Returns a path on disk for APIs that require a filesystem path (fonts, icons).
        /// Writes pack entries to a temp file when needed.
        static std::filesystem::path MaterializeLooseFile(const std::string &relativePath);

    private:
        static std::string NormalizeRelativePath(std::string relativePath);
        static std::optional<std::vector<uint8_t>> ReadLooseBytes(const std::string &relativePath);
    };
}
