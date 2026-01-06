#pragma once

#include "Himii/Asset/AssetMetadata.h"
#include "Himii/Core/Core.h"

#include <map>
#include <unordered_map>

namespace Himii
{

    using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager() = default;

        // 核心 API
        Ref<Asset> GetAsset(AssetHandle handle);

        // 编辑器使用：导入新文件到系统
        AssetHandle ImportAsset(const std::filesystem::path &filepath);

        // 检查是否存在
        bool IsAssetHandleValid(AssetHandle handle) const;
        bool IsAssetLoaded(AssetHandle handle) const;

        const AssetRegistry &GetAssetRegistry() const
        {
            return m_AssetRegistry;
        }

        // 临时辅助：根据扩展名猜测类型
        static AssetType GetAssetTypeFromExtension(const std::string &extension);

        void SerializeAssetRegistry();

        bool DeserializeAssetRegistry();

    private:
        AssetRegistry m_AssetRegistry;
        std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
    };
} // namespace Himii
