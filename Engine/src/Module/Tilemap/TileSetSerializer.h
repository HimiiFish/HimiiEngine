#pragma once

#include "Module/Tilemap/TileSet.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    class TileSetSerializer
    {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<TileSet> &tileSet);
        static Ref<TileSet> Deserialize(const std::filesystem::path &filepath);
    };
}
