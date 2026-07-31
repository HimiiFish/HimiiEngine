#pragma once

#include "Resource/Sprite.h"
#include <filesystem>

namespace Himii
{

    class TextureImportSerializer
    {
    public:
        static std::filesystem::path GetMetaPath(const std::filesystem::path& textureFilesystemPath);

        static bool MetaExists(const std::filesystem::path& textureFilesystemPath);

        static bool Deserialize(const std::filesystem::path& textureFilesystemPath,
                                TextureImportData& outImportData);

        static bool Serialize(const std::filesystem::path& textureFilesystemPath,
                              const TextureImportData& importData);
    };

} // namespace Himii
