#pragma once

#include "EngineCore/Core/UUID.h"
#include <string>
#include <string_view>

namespace Himii {

    using AssetHandle = UUID;

    enum class AssetType : uint16_t
    {
        None = 0,
        Scene,
        Texture2D,
        SpriteAnimation,
        TileSet,
        TileMap,
        ParticleEmitter,
        Prefab,
        Font,
        SoundAsset,
        Mesh,
        Material,
        Shader
    };

    class Asset {
    public:
        AssetHandle Handle; // 自动生成 UUID

        virtual ~Asset() = default;
        virtual AssetType GetType() const = 0;

        // 辅助转字符串，用于调试或序列化
        static std::string AssetTypeToString(AssetType type)
        {
            switch (type)
            {
                case AssetType::None:            return "None";
                case AssetType::Scene:           return "Scene";
                case AssetType::Texture2D:       return "Texture2D";
                case AssetType::SpriteAnimation: return "SpriteAnimation";
                case AssetType::TileSet:         return "TileSet";
                case AssetType::TileMap:         return "TileMap";
                case AssetType::ParticleEmitter: return "ParticleEmitter";
                case AssetType::Prefab:          return "Prefab";
                case AssetType::Font:            return "Font";
                case AssetType::SoundAsset:      return "SoundAsset";
                case AssetType::Mesh:            return "Mesh";
                case AssetType::Material:        return "Material";
                case AssetType::Shader:          return "Shader";
            }
            return "None";
        }

        static AssetType AssetTypeFromString(const std::string &assetType)
        {
            if (assetType == "None")            return AssetType::None;
            if (assetType == "Scene")           return AssetType::Scene;
            if (assetType == "Texture2D")       return AssetType::Texture2D;
            if (assetType == "SpriteAnimation") return AssetType::SpriteAnimation;
            if (assetType == "TileSet")         return AssetType::TileSet;
            if (assetType == "TileMap")         return AssetType::TileMap;
            if (assetType == "ParticleEmitter") return AssetType::ParticleEmitter;
            if (assetType == "Prefab")          return AssetType::Prefab;
            if (assetType == "Font")            return AssetType::Font;
            if (assetType == "SoundAsset")      return AssetType::SoundAsset;
            if (assetType == "Mesh")            return AssetType::Mesh;
            if (assetType == "Material")        return AssetType::Material;
            if (assetType == "Shader")          return AssetType::Shader;

            return AssetType::None;
        }
    };
}
