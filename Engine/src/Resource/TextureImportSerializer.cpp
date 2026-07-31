#include "Resource/TextureImportSerializer.h"

#include "EngineCore/Core/Log.h"

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

namespace Himii
{

    std::filesystem::path TextureImportSerializer::GetMetaPath(const std::filesystem::path& textureFilesystemPath)
    {
        return textureFilesystemPath.string() + ".meta";
    }

    bool TextureImportSerializer::MetaExists(const std::filesystem::path& textureFilesystemPath)
    {
        return std::filesystem::exists(GetMetaPath(textureFilesystemPath));
    }

    static TextureSpriteMode SpriteModeFromString(const std::string& value)
    {
        if (value == "None")
            return TextureSpriteMode::None;
        if (value == "Multiple")
            return TextureSpriteMode::Multiple;
        return TextureSpriteMode::Single;
    }

    static std::string SpriteModeToString(TextureSpriteMode mode)
    {
        switch (mode)
        {
            case TextureSpriteMode::None: return "None";
            case TextureSpriteMode::Multiple: return "Multiple";
            default: return "Single";
        }
    }

    static glm::ivec4 ReadPixelRect(const YAML::Node& node)
    {
        glm::ivec4 rect{0, 0, 0, 0};
        if (!node || !node.IsSequence() || node.size() < 4)
            return rect;
        rect.x = node[0].as<int>();
        rect.y = node[1].as<int>();
        rect.z = node[2].as<int>();
        rect.w = node[3].as<int>();
        return rect;
    }

    static glm::vec2 ReadVec2(const YAML::Node& node, const glm::vec2& defaultValue = {0.5f, 0.5f})
    {
        if (!node || !node.IsSequence() || node.size() < 2)
            return defaultValue;
        return {node[0].as<float>(), node[1].as<float>()};
    }

    static glm::ivec2 ReadIVec2(const YAML::Node& node, const glm::ivec2& defaultValue = {0, 0})
    {
        if (!node || !node.IsSequence() || node.size() < 2)
            return defaultValue;
        return {node[0].as<int>(), node[1].as<int>()};
    }

    bool TextureImportSerializer::Deserialize(const std::filesystem::path& textureFilesystemPath,
                                              TextureImportData& outImportData)
    {
        const std::filesystem::path metaPath = GetMetaPath(textureFilesystemPath);
        if (!std::filesystem::exists(metaPath))
            return false;

        try
        {
            YAML::Node data = YAML::LoadFile(metaPath.string());
            if (!data["AssetType"] || data["AssetType"].as<std::string>() != "TextureImport")
                return false;

            if (data["TextureHandle"])
                outImportData.TextureHandle = data["TextureHandle"].as<uint64_t>();

            if (data["SpriteMode"])
                outImportData.SpriteMode = SpriteModeFromString(data["SpriteMode"].as<std::string>());

            if (data["PixelsPerUnit"])
                outImportData.PixelsPerUnit = data["PixelsPerUnit"].as<uint32_t>();

            outImportData.GridCellSize = ReadIVec2(data["GridCellSize"]);
            outImportData.GridOffset = ReadIVec2(data["GridOffset"]);
            outImportData.GridPadding = ReadIVec2(data["GridPadding"]);

            outImportData.Sprites.clear();
            if (data["Sprites"])
            {
                for (const auto& spriteNode : data["Sprites"])
                {
                    SpriteDefinition sprite;
                    if (spriteNode["Handle"])
                        sprite.Handle = spriteNode["Handle"].as<uint64_t>();
                    if (spriteNode["Name"])
                        sprite.Name = spriteNode["Name"].as<std::string>();
                    sprite.PixelRect = ReadPixelRect(spriteNode["PixelRect"]);
                    sprite.Pivot = ReadVec2(spriteNode["Pivot"]);
                    if (spriteNode["Border"])
                    {
                        const auto& borderNode = spriteNode["Border"];
                        if (borderNode.IsSequence() && borderNode.size() >= 4)
                        {
                            sprite.Border = {
                                borderNode[0].as<float>(),
                                borderNode[1].as<float>(),
                                borderNode[2].as<float>(),
                                borderNode[3].as<float>()
                            };
                        }
                    }
                    outImportData.Sprites.push_back(sprite);
                }
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            HIMII_CORE_ERROR("Failed to deserialize texture meta {0}: {1}",
                             metaPath.string(), exception.what());
            return false;
        }
    }

    bool TextureImportSerializer::Serialize(const std::filesystem::path& textureFilesystemPath,
                                            const TextureImportData& importData)
    {
        const std::filesystem::path metaPath = GetMetaPath(textureFilesystemPath);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetType" << YAML::Value << "TextureImport";
        out << YAML::Key << "TextureHandle" << YAML::Value << static_cast<uint64_t>(importData.TextureHandle);
        out << YAML::Key << "SpriteMode" << YAML::Value << SpriteModeToString(importData.SpriteMode);
        out << YAML::Key << "PixelsPerUnit" << YAML::Value << importData.PixelsPerUnit;

        out << YAML::Key << "GridCellSize" << YAML::Value
            << YAML::Flow << YAML::BeginSeq
            << importData.GridCellSize.x << importData.GridCellSize.y << YAML::EndSeq;
        out << YAML::Key << "GridOffset" << YAML::Value
            << YAML::Flow << YAML::BeginSeq
            << importData.GridOffset.x << importData.GridOffset.y << YAML::EndSeq;
        out << YAML::Key << "GridPadding" << YAML::Value
            << YAML::Flow << YAML::BeginSeq
            << importData.GridPadding.x << importData.GridPadding.y << YAML::EndSeq;

        out << YAML::Key << "Sprites" << YAML::Value << YAML::BeginSeq;
        for (const SpriteDefinition& sprite : importData.Sprites)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Handle" << YAML::Value << static_cast<uint64_t>(sprite.Handle);
            out << YAML::Key << "Name" << YAML::Value << sprite.Name;
            out << YAML::Key << "PixelRect" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << sprite.PixelRect.x << sprite.PixelRect.y
                << sprite.PixelRect.z << sprite.PixelRect.w << YAML::EndSeq;
            out << YAML::Key << "Pivot" << YAML::Value
                << YAML::Flow << YAML::BeginSeq << sprite.Pivot.x << sprite.Pivot.y << YAML::EndSeq;
            out << YAML::Key << "Border" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << sprite.Border.x << sprite.Border.y
                << sprite.Border.z << sprite.Border.w << YAML::EndSeq;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream outputStream(metaPath);
        if (!outputStream.is_open())
        {
            HIMII_CORE_ERROR("Failed to write texture meta: {0}", metaPath.string());
            return false;
        }
        outputStream << out.c_str();
        return true;
    }

} // namespace Himii
