#include "Hepch.h"
#include "Resource/ResourceSystem.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    Ref<AssetManager> ResourceSystem::s_BoundAssetManager;

    void ResourceSystem::Bind(const Ref<AssetManager> &assetManager)
    {
        s_BoundAssetManager = assetManager;
    }

    void ResourceSystem::Unbind()
    {
        s_BoundAssetManager.reset();
    }

    bool ResourceSystem::IsBound()
    {
        return static_cast<bool>(s_BoundAssetManager);
    }

    Ref<AssetManager> ResourceSystem::GetAssetManager()
    {
        return s_BoundAssetManager;
    }

    AssetManager &ResourceSystem::GetAssetManagerChecked()
    {
        HIMII_CORE_ASSERT(s_BoundAssetManager, "ResourceSystem is not bound to an AssetManager.");
        return *s_BoundAssetManager;
    }

    Ref<Asset> ResourceSystem::GetAsset(AssetHandle handle)
    {
        if (!s_BoundAssetManager)
            return nullptr;
        return s_BoundAssetManager->GetAsset(handle);
    }

    bool ResourceSystem::IsAssetHandleValid(AssetHandle handle)
    {
        if (!s_BoundAssetManager)
            return false;
        return s_BoundAssetManager->IsAssetHandleValid(handle);
    }

    bool ResourceSystem::IsAssetLoaded(AssetHandle handle)
    {
        if (!s_BoundAssetManager)
            return false;
        return s_BoundAssetManager->IsAssetLoaded(handle);
    }

    AssetHandle ResourceSystem::ImportAsset(const std::filesystem::path &filepath)
    {
        if (!s_BoundAssetManager)
            return 0;
        return s_BoundAssetManager->ImportAsset(filepath);
    }

    void ResourceSystem::SerializeAssetRegistry()
    {
        if (s_BoundAssetManager)
            s_BoundAssetManager->SerializeAssetRegistry();
    }

    bool ResourceSystem::DeserializeAssetRegistry()
    {
        if (!s_BoundAssetManager)
            return false;
        return s_BoundAssetManager->DeserializeAssetRegistry();
    }
}
