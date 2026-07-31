#pragma once

#include "Module/Tilemap/TileMapData.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    class TileMapDataSerializer
    {
    public:
        static void Serialize(const std::filesystem::path &filepath, const Ref<TileMapData> &tileMapData);
        static Ref<TileMapData> Deserialize(const std::filesystem::path &filepath);
    };
}
