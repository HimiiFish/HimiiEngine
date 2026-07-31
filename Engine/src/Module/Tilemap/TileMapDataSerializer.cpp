#include "Hepch.h"
#include "Module/Tilemap/TileMapDataSerializer.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{

    void TileMapDataSerializer::Serialize(const std::filesystem::path &filepath, const Ref<TileMapData> &tileMapData)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "TileMap";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)tileMapData->Handle;
        out << YAML::Key << "TileSetHandle" << YAML::Value << (uint64_t)tileMapData->GetTileSetHandle();
        out << YAML::Key << "CellSize" << YAML::Value << tileMapData->GetCellSize();

        out << YAML::Key << "Chunks" << YAML::Value << YAML::BeginSeq;
        for (const auto& [chunkKey, chunk] : tileMapData->GetChunks())
        {
            bool hasAnyTile = false;
            for (uint16_t tileIdentifier : chunk.Tiles)
            {
                if (tileIdentifier != 0)
                {
                    hasAnyTile = true;
                    break;
                }
            }
            if (!hasAnyTile)
                continue;

            out << YAML::BeginMap;
            out << YAML::Key << "ChunkX" << YAML::Value << chunkKey.ChunkX;
            out << YAML::Key << "ChunkY" << YAML::Value << chunkKey.ChunkY;
            out << YAML::Key << "Tiles" << YAML::Value << YAML::Flow << YAML::BeginSeq;
            for (uint16_t tileIdentifier : chunk.Tiles)
                out << (int)tileIdentifier;
            out << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<TileMapData> TileMapDataSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open TileMapData file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream strStream;
            strStream << stream.rdbuf();

            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "TileMap")
            {
                HIMII_CORE_ERROR("Invalid TileMapData asset: {0}", filepath.string());
                return nullptr;
            }

            Ref<TileMapData> tileMapData = CreateRef<TileMapData>();

            if (data["Handle"])
                tileMapData->Handle = data["Handle"].as<uint64_t>();

            if (data["TileSetHandle"])
                tileMapData->SetTileSetHandle(data["TileSetHandle"].as<uint64_t>());

            uint32_t halfWidth = 0, halfHeight = 0;
            if (data["HalfWidth"] && data["HalfHeight"])
            {
                halfWidth = data["HalfWidth"].as<uint32_t>();
                halfHeight = data["HalfHeight"].as<uint32_t>();
            }
            else if (data["Width"] && data["Height"])
            {
                uint32_t w = data["Width"].as<uint32_t>();
                uint32_t h = data["Height"].as<uint32_t>();
                halfWidth = (w > 0) ? (w - 1) / 2 : 0;
                halfHeight = (h > 0) ? (h - 1) / 2 : 0;
            }

            if (data["CellSize"])
                tileMapData->SetCellSize(data["CellSize"].as<float>());

            if (data["Chunks"] && data["Chunks"].IsSequence())
            {
                for (const auto& chunkNode : data["Chunks"])
                {
                    if (!chunkNode["ChunkX"] || !chunkNode["ChunkY"] || !chunkNode["Tiles"])
                        continue;

                    const int32_t chunkX = chunkNode["ChunkX"].as<int32_t>();
                    const int32_t chunkY = chunkNode["ChunkY"].as<int32_t>();
                    const auto& tilesSequence = chunkNode["Tiles"];

                    for (int32_t localY = 0; localY < TileMapChunkSize; ++localY)
                    {
                        for (int32_t localX = 0; localX < TileMapChunkSize; ++localX)
                        {
                            const size_t arrayIndex =
                                    static_cast<size_t>(localX)
                                    + static_cast<size_t>(localY) * TileMapChunkSize;
                            if (arrayIndex >= tilesSequence.size())
                                continue;

                            const uint16_t tileIdentifier =
                                    (uint16_t)tilesSequence[arrayIndex].as<int>();
                            if (tileIdentifier == 0)
                                continue;

                            const int32_t tileX = chunkX * TileMapChunkSize + localX;
                            const int32_t tileY = chunkY * TileMapChunkSize + localY;
                            tileMapData->SetTile(tileX, tileY, tileIdentifier);
                        }
                    }
                }
            }
            else if (data["Tiles"] && data["Tiles"].IsSequence())
            {
                std::vector<uint16_t> denseTiles;
                denseTiles.reserve(data["Tiles"].size());
                for (const auto& tileNode : data["Tiles"])
                    denseTiles.push_back((uint16_t)tileNode.as<int>());

                tileMapData->ImportLegacyDenseTiles(halfWidth, halfHeight, denseTiles);
            }

            return tileMapData;
        }
        catch (const YAML::Exception &e)
        {
            HIMII_CORE_ERROR("Failed to deserialize TileMapData '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

} // namespace Himii

