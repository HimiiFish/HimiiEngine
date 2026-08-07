#pragma once

#include "Resource/AssetMetadata.h"
#include "Resource/Sprite.h"
#include "EngineCore/Core/Core.h"

#include <map>
#include <unordered_map>

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

        /// 按材质参数中的贴图相对路径恢复 / 同步 Handle（保存与加载时复用）。
        void ResolveMaterialAlbedoTextureReference(MaterialAsset &materialAsset);

    private:
        /// 旧 Registry 若仍指向网格源文件，则确保存在 `.hmesh` 并将条目迁移到产品路径。
        bool EnsureStaticMeshProductForRegistryEntry(AssetHandle handle, AssetMetadata &metadata);

        void RegisterSpritesFromImportData(const TextureImportData& importData);
        void UnregisterSpritesForTexture(AssetHandle textureHandle);

        AssetRegistry m_AssetRegistry;
        std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;

        std::unordered_map<AssetHandle, TextureImportData> m_TextureImportData;
        std::unordered_map<AssetHandle, SpriteRegistryEntry> m_SpriteRegistry;
    };
} // namespace Himii
