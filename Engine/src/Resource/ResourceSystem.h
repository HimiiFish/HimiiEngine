#pragma once

#include "Resource/AssetManager.h"
#include "EngineCore/Core/Core.h"

#include <filesystem>

namespace Himii
{
    /// Resource 对外门面：绑定的 AssetManager 为权威；静态糖衣便于全局调用。
    /// 领域序列化仍由各 Module 实现；本类负责绑定与通用加载/查询入口。
    class ResourceSystem
    {
    public:
        static void Bind(const Ref<AssetManager> &assetManager);
        static void Unbind();
        static bool IsBound();

        static Ref<AssetManager> GetAssetManager();
        static AssetManager &GetAssetManagerChecked();

        static Ref<Asset> GetAsset(AssetHandle handle);
        static bool IsAssetHandleValid(AssetHandle handle);
        static bool IsAssetLoaded(AssetHandle handle);

        static AssetHandle ImportAsset(const std::filesystem::path &filepath);

        static void SerializeAssetRegistry();
        static bool DeserializeAssetRegistry();

    private:
        static Ref<AssetManager> s_BoundAssetManager;
    };
}
