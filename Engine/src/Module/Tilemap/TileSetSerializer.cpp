#include "Hepch.h"
#include "Module/Tilemap/TileSetSerializer.h"
#include "EngineCore/Core/Log.h"
#include <fstream>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{

    void TileSetSerializer::Serialize(const std::filesystem::path &filepath, const Ref<TileSet> &tileSet)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "TileSet";
        out << YAML::Key << "Handle" << YAML::Value << (uint64_t)tileSet->Handle;

        // Atlas Sources
        out << YAML::Key << "AtlasSources" << YAML::Value << YAML::BeginSeq;
        for (const auto &source : tileSet->GetAtlasSources())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)source.TextureHandle;
            out << YAML::Key << "TileSize" << YAML::Value << source.TileSize;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        // Tile Definitions
        out << YAML::Key << "TileDefs" << YAML::Value << YAML::BeginSeq;
        for (const auto &[id, def] : tileSet->GetTileDefs())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "ID" << YAML::Value << (int)def.ID;
            out << YAML::Key << "SourceType" << YAML::Value << (int)def.SourceType;

            if (def.SourceType == TileSourceType::Atlas)
            {
                out << YAML::Key << "AtlasSourceIndex" << YAML::Value << def.AtlasSourceIndex;
                out << YAML::Key << "AtlasCoordsX" << YAML::Value << def.AtlasCoords.x;
                out << YAML::Key << "AtlasCoordsY" << YAML::Value << def.AtlasCoords.y;
            }
            else
            {
                out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)def.IndividualTextureHandle;
            }

            out << YAML::Key << "Tint" << YAML::Value << YAML::Flow << YAML::BeginSeq
                << def.Tint.r << def.Tint.g << def.Tint.b << def.Tint.a << YAML::EndSeq;
            out << YAML::Key << "Collidable" << YAML::Value << def.Collidable;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }

    Ref<TileSet> TileSetSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        try
        {
            std::ifstream stream(filepath);
            if (!stream.is_open())
            {
                HIMII_CORE_ERROR("Failed to open TileSet file: {0}", filepath.string());
                return nullptr;
            }

            std::stringstream strStream;
            strStream << stream.rdbuf();

            YAML::Node data = YAML::Load(strStream.str());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "TileSet")
            {
                HIMII_CORE_ERROR("Invalid TileSet asset: {0}", filepath.string());
                return nullptr;
            }

            Ref<TileSet> tileSet = CreateRef<TileSet>();

            if (data["Handle"])
                tileSet->Handle = data["Handle"].as<uint64_t>();

            // Atlas Sources
            if (data["AtlasSources"])
            {
                for (auto sourceNode : data["AtlasSources"])
                {
                    TileAtlasSource source;
                    source.TextureHandle = sourceNode["TextureHandle"].as<uint64_t>();
                    source.TileSize = sourceNode["TileSize"].as<uint32_t>();
                    tileSet->AddAtlasSource(source);
                }
            }

            // Tile Definitions
            if (data["TileDefs"])
            {
                for (auto defNode : data["TileDefs"])
                {
                    TileDef def;
                    def.ID = (uint16_t)defNode["ID"].as<int>();
                    def.SourceType = (TileSourceType)defNode["SourceType"].as<int>();

                    if (def.SourceType == TileSourceType::Atlas)
                    {
                        def.AtlasSourceIndex = defNode["AtlasSourceIndex"].as<uint32_t>();
                        def.AtlasCoords.x = defNode["AtlasCoordsX"].as<int>();
                        def.AtlasCoords.y = defNode["AtlasCoordsY"].as<int>();
                    }
                    else
                    {
                        if (defNode["TextureHandle"])
                            def.IndividualTextureHandle = defNode["TextureHandle"].as<uint64_t>();
                    }

                    if (defNode["Tint"] && defNode["Tint"].IsSequence() && defNode["Tint"].size() == 4)
                    {
                        def.Tint.r = defNode["Tint"][0].as<float>();
                        def.Tint.g = defNode["Tint"][1].as<float>();
                        def.Tint.b = defNode["Tint"][2].as<float>();
                        def.Tint.a = defNode["Tint"][3].as<float>();
                    }

                    if (defNode["Collidable"])
                        def.Collidable = defNode["Collidable"].as<bool>();

                    tileSet->AddTileDef(def);
                }
            }

            return tileSet;
        }
        catch (const YAML::Exception &e)
        {
            HIMII_CORE_ERROR("Failed to deserialize TileSet '{0}': {1}", filepath.string(), e.what());
            return nullptr;
        }
    }

} // namespace Himii

