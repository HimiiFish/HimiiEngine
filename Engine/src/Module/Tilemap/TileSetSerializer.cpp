#include "Hepch.h"
#include "Module/Tilemap/TileSetSerializer.h"
#include "EngineCore/Core/Log.h"
#include <algorithm>
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

        out << YAML::Key << "RuleTiles" << YAML::Value << YAML::BeginSeq;
        for (const auto &[ruleTileIdentifier, ruleTileDefinition] : tileSet->GetRuleTileDefinitions())
        {
            out << YAML::BeginMap;
            out << YAML::Key << "ID" << YAML::Value << (int)ruleTileDefinition.Identifier;
            out << YAML::Key << "DisplayName" << YAML::Value << ruleTileDefinition.DisplayName;
            out << YAML::Key << "Collidable" << YAML::Value << ruleTileDefinition.Collidable;

            out << YAML::Key << "DefaultOutputTileIdentifiers" << YAML::Value << YAML::Flow
                << YAML::BeginSeq;
            for (uint16_t outputTileIdentifier : ruleTileDefinition.DefaultOutputTileIdentifiers)
                out << (int)outputTileIdentifier;
            out << YAML::EndSeq;

            out << YAML::Key << "Rules" << YAML::Value << YAML::BeginSeq;
            for (const RuleTileRule &rule : ruleTileDefinition.Rules)
            {
                out << YAML::BeginMap;
                out << YAML::Key << "MatchTransform" << YAML::Value << (int)rule.MatchTransform;
                out << YAML::Key << "NeighborConditions" << YAML::Value << YAML::Flow << YAML::BeginSeq;
                for (uint32_t neighborIndex = 0; neighborIndex < RuleTileNeighborCount; ++neighborIndex)
                    out << (int)rule.NeighborConditions[neighborIndex];
                out << YAML::EndSeq;

                out << YAML::Key << "OutputTileIdentifiers" << YAML::Value << YAML::Flow
                    << YAML::BeginSeq;
                for (uint16_t outputTileIdentifier : rule.OutputTileIdentifiers)
                    out << (int)outputTileIdentifier;
                out << YAML::EndSeq;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
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

            if (data["RuleTiles"])
            {
                for (auto ruleTileNode : data["RuleTiles"])
                {
                    RuleTileDefinition ruleTileDefinition;
                    if (ruleTileNode["ID"])
                        ruleTileDefinition.Identifier = (uint16_t)ruleTileNode["ID"].as<int>();
                    if (ruleTileNode["DisplayName"])
                        ruleTileDefinition.DisplayName = ruleTileNode["DisplayName"].as<std::string>();
                    if (ruleTileNode["Collidable"])
                        ruleTileDefinition.Collidable = ruleTileNode["Collidable"].as<bool>();

                    if (ruleTileNode["DefaultOutputTileIdentifiers"]
                        && ruleTileNode["DefaultOutputTileIdentifiers"].IsSequence())
                    {
                        for (auto outputNode : ruleTileNode["DefaultOutputTileIdentifiers"])
                            ruleTileDefinition.DefaultOutputTileIdentifiers.push_back(
                                    (uint16_t)outputNode.as<int>());
                    }

                    if (ruleTileNode["Rules"] && ruleTileNode["Rules"].IsSequence())
                    {
                        for (auto ruleNode : ruleTileNode["Rules"])
                        {
                            RuleTileRule rule;
                            if (ruleNode["MatchTransform"])
                            {
                                const int matchTransformValue = ruleNode["MatchTransform"].as<int>();
                                if (matchTransformValue >= 0 && matchTransformValue <= 4)
                                    rule.MatchTransform =
                                            static_cast<RuleTileMatchTransform>(matchTransformValue);
                            }

                            if (ruleNode["NeighborConditions"] && ruleNode["NeighborConditions"].IsSequence())
                            {
                                const uint32_t conditionCount = std::min(
                                        RuleTileNeighborCount,
                                        static_cast<uint32_t>(ruleNode["NeighborConditions"].size()));
                                for (uint32_t neighborIndex = 0; neighborIndex < conditionCount;
                                     ++neighborIndex)
                                {
                                    const int conditionValue =
                                            ruleNode["NeighborConditions"][neighborIndex].as<int>();
                                    if (conditionValue >= 0 && conditionValue <= 2)
                                    {
                                        rule.NeighborConditions[neighborIndex] =
                                                static_cast<RuleTileNeighborCondition>(conditionValue);
                                    }
                                }
                            }

                            if (ruleNode["OutputTileIdentifiers"]
                                && ruleNode["OutputTileIdentifiers"].IsSequence())
                            {
                                for (auto outputNode : ruleNode["OutputTileIdentifiers"])
                                    rule.OutputTileIdentifiers.push_back((uint16_t)outputNode.as<int>());
                            }

                            ruleTileDefinition.Rules.push_back(std::move(rule));
                        }
                    }

                    tileSet->AddRuleTileDefinition(ruleTileDefinition);
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

