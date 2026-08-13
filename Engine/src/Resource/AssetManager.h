#pragma once

#include "Resource/AssetMetadata.h"
#include "Resource/Sprite.h"
#include "EngineCore/Core/Core.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

namespace Himii
{
    class MaterialAsset;

    using AssetRegistry = std::map<AssetHandle, AssetMetadata>;

    struct SpriteRegistryEntry
    {
        AssetHandle TextureHandle = 0;
        uint32_t SpriteIndex = 0;
    };

    class AssetManager {
    public:
        AssetManager();
        ~AssetManager() = default;

        Ref<Asset> GetAsset(AssetHandle handle);

        AssetHandle ImportAsset(const std::filesystem::path &filepath);

        /// 仅查找 Registry，不创建新条目。
        AssetHandle FindAssetHandleByFilePath(const std::filesystem::path &filepath) const;

        bool IsAssetHandleValid(AssetHandle handle) const;
        bool IsAssetLoaded(AssetHandle handle) const;
        bool IsSpriteHandle(AssetHandle handle) const;

        const AssetRegistry &GetAssetRegistry() const
        {
            return m_AssetRegistry;
        }

        static AssetType GetAssetTypeFromExtension(const std::string &extension);

        void SerializeAssetRegistry();
        bool DeserializeAssetRegistry();

        TextureImportData& GetOrCreateTextureImportData(AssetHandle textureHandle);
        const TextureImportData* GetTextureImportData(AssetHandle textureHandle) const;

        bool LoadTextureImportData(AssetHandle textureHandle);
        bool SaveTextureImportData(AssetHandle textureHandle);

        void EnsureDefaultTextureMeta(AssetHandle textureHandle);
        void ApplyGridSliceToTexture(AssetHandle textureHandle);

        const SpriteDefinition* GetSpriteDefinition(AssetHandle spriteHandle) const;
        SpriteResolved ResolveSprite(AssetHandle spriteHandle);
        SpriteResolved ResolveSpriteFromTexture(AssetHandle textureHandle, uint32_t spriteIndex = 0);

        const std::vector<SpriteDefinition>& GetSpritesForTexture(AssetHandle textureHandle);
        AssetHandle GetDefaultSpriteHandleForTexture(AssetHandle textureHandle);

        AssetHandle GetTextureHandleForSprite(AssetHandle spriteAssetHandle) const;

        void UnloadAsset(AssetHandle handle);

        /// 本会话内该 Handle 已加载失败；跳过后续反序列化以避免重复日志。
        bool HasCachedAssetLoadFailure(AssetHandle handle) const;

        /// Reimport / 成功导入后清除失败缓存，允许再次加载。
        void ClearCachedAssetLoadFailure(AssetHandle handle);

        /// 按材质参数中的贴图相对路径恢复 / 同步 Handle（Albedo / Metallic / Roughness）。
        void ResolveMaterialAlbedoTextureReference(MaterialAsset &materialAsset);

    private:
        void RegisterSpritesFromImportData(const TextureImportData& importData);
        void UnregisterSpritesForTexture(AssetHandle textureHandle);

        AssetRegistry m_AssetRegistry;
        std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

        std::unordered_map<AssetHandle, TextureImportData> m_TextureImportData;
        std::unordered_map<AssetHandle, SpriteRegistryEntry> m_SpriteRegistry;
        std::unordered_set<AssetHandle> m_FailedAssetLoadHandles;
    };
} // namespace Himii
