#pragma once

#include "Resource/Asset.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    /// 领域资产加载器：由各 Module 实现并通过 AssetSerializerRegistry 注册。
    class IAssetSerializer
    {
    public:
        virtual ~IAssetSerializer() = default;

        virtual AssetType GetAssetType() const = 0;
        virtual Ref<Asset> Deserialize(const std::filesystem::path &filepath) = 0;
    };
}
