#pragma once

#include "Asset.h"
#include <filesystem>

namespace Himii {

    struct AssetMetadata
    {
        AssetHandle Handle = 0;
        AssetType Type = AssetType::None;
        std::filesystem::path FilePath;
        bool IsLoaded = false;
        
        operator bool() const { return Type != AssetType::None; }
    };

}
