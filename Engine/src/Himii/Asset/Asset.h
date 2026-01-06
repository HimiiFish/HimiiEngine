#pragma once

#include "Himii/Core/UUID.h"
#include <string>
#include <string_view>

namespace Himii {

    using AssetHandle = UUID;

    enum class AssetType : uint16_t
    {
        None = 0,
        Scene,
        Texture2D,
        SpriteAnimation
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
                case AssetType::None:
                    return "None";
                case AssetType::Scene:
                    return "Scene";
                case AssetType::Texture2D:
                    return "Texture2D";
                case AssetType::SpriteAnimation:
                    return "SpriteAnimation";
            }
            return "None";
        }

        static AssetType AssetTypeFromString(const std::string &assetType)
        {
            if (assetType == "None")
                return AssetType::None;
            if (assetType == "Scene")
                return AssetType::Scene;
            if (assetType == "Texture2D")
                return AssetType::Texture2D;
            if (assetType == "SpriteAnimation")
                return AssetType::SpriteAnimation;

            return AssetType::None;
        }
    };
}
