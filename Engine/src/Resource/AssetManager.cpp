#include "Resource/AssetManager.h"
#include "EngineCore/Core/Log.h"
#include "Project/Project.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Render/Mesh/GltfMeshImporter.h"
#include "Resource/AssetSerializerRegistry.h"
#include "Resource/TextureImportSerializer.h"
#include "Resource/SpriteSheetUtility.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <algorithm>

namespace Himii
{

    AssetManager::AssetManager()
    {
    }

    Ref<Asset> AssetManager::GetAsset(AssetHandle handle)
    {
        if (IsAssetLoaded(handle))
            return m_LoadedAssets.at(handle);

        const auto metadataIterator = m_AssetRegistry.find(handle);
        if (metadataIterator == m_AssetRegistry.end() || !metadataIterator->second)
            return nullptr;

        const AssetMetadata& metadata = metadataIterator->second;

        Ref<Asset> asset = nullptr;
        std::filesystem::path filesystemPath = Project::GetAssetFileSystemPath(metadata.FilePath);

        if (AssetSerializerRegistry::HasSerializer(metadata.Type))
        {
            asset = AssetSerializerRegistry::Deserialize(metadata.Type, filesystemPath);
            if (asset && metadata.Type == AssetType::Texture2D)
                EnsureDefaultTextureMeta(handle);
        }

        if (asset)
        {
            asset->Handle = handle;
            m_LoadedAssets[handle] = asset;
            m_AssetRegistry[handle].IsLoaded = true;
        }

        return asset;
    }

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
    {
        std::filesystem::path relativePath = filepath;

        for (auto &[handle, metadata]: m_AssetRegistry)
        {
            if (metadata.FilePath.generic_string() == filepath.generic_string())
                return handle;
        }

        HIMII_CORE_INFO("Importing NEW Asset: {0}", filepath.generic_string());

        AssetMetadata metadata;
        metadata.Handle = AssetHandle();
        metadata.FilePath = relativePath;
        metadata.Type = GetAssetTypeFromExtension(relativePath.extension().string());

        if (metadata.Type != AssetType::None)
        {
            m_AssetRegistry[metadata.Handle] = metadata;

            if (metadata.Type == AssetType::Texture2D)
                EnsureDefaultTextureMeta(metadata.Handle);
            else if (metadata.Type == AssetType::Mesh)
                GltfMeshImporter::ImportCompanionAssets(*this, relativePath, metadata.Handle);

            return metadata.Handle;
        }

        return 0;
    }

    bool AssetManager::IsAssetHandleValid(AssetHandle handle) const
    {
        return handle != 0 && m_AssetRegistry.find(handle) != m_AssetRegistry.end();
    }

    bool AssetManager::IsAssetLoaded(AssetHandle handle) const
    {
        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    bool AssetManager::IsSpriteHandle(AssetHandle handle) const
    {
        return m_SpriteRegistry.find(handle) != m_SpriteRegistry.end();
    }

    AssetType AssetManager::GetAssetTypeFromExtension(const std::string &extension)
    {
        return AssetSerializerRegistry::GetAssetTypeFromExtension(extension);
    }

    void AssetManager::SerializeAssetRegistry()
    {
        auto path = Project::GetAssetRegistryPath();

        HIMII_CORE_INFO("Serializing AssetRegistry to: {0}, Count: {1}", path.string(), m_AssetRegistry.size());

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "AssetRegistry" << YAML::Value << YAML::BeginSeq;

        for (const auto &[handle, metadata]: m_AssetRegistry)
        {
            out << YAML::BeginMap;
            out << YAML::Key << "Handle" << YAML::Value << (uint64_t)handle;
            out << YAML::Key << "FilePath" << YAML::Value << std::string(metadata.FilePath.generic_string());
            out << YAML::Key << "Type" << YAML::Value << Asset::AssetTypeToString(metadata.Type);
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::ofstream outputFile(path);
        outputFile << out.c_str();
    }

    bool AssetManager::DeserializeAssetRegistry()
    {
        auto path = Project::GetAssetRegistryPath();

        HIMII_CORE_INFO("Deserializing AssetRegistry from: {0}", path.string());

        if (!std::filesystem::exists(path))
        {
            HIMII_CORE_WARNING("Asset Registry file does not exist!");
            return false;
        }

        YAML::Node data;
        try
        {
            data = YAML::LoadFile(path.string());
        }
        catch (std::exception &exception)
        {
            HIMII_CORE_ERROR("Failed to load asset registry: {0}", exception.what());
            return false;
        }

        auto registryNode = data["AssetRegistry"];
        if (!registryNode)
        {
            HIMII_CORE_WARNING("AssetRegistry node missing in yaml!");
            return false;
        }

        m_AssetRegistry.clear();
        m_LoadedAssets.clear();
        m_TextureImportData.clear();
        m_SpriteRegistry.clear();

        for (auto node: registryNode)
        {
            AssetHandle handle = node["Handle"].as<uint64_t>();
            auto &metadata = m_AssetRegistry[handle];
            metadata.Handle = handle;
            metadata.FilePath = node["FilePath"].as<std::string>();
            metadata.Type = Asset::AssetTypeFromString(node["Type"].as<std::string>());
        }

        for (const auto &[handle, metadata] : m_AssetRegistry)
        {
            if (metadata.Type == AssetType::Texture2D)
                LoadTextureImportData(handle);
        }

        HIMII_CORE_INFO("Loaded AssetRegistry. Total assets: {0}", m_AssetRegistry.size());
        return true;
    }

    void AssetManager::RegisterSpritesFromImportData(const TextureImportData& importData)
    {
        UnregisterSpritesForTexture(importData.TextureHandle);

        for (uint32_t spriteIndex = 0; spriteIndex < importData.Sprites.size(); ++spriteIndex)
        {
            const SpriteDefinition& sprite = importData.Sprites[spriteIndex];
            if (sprite.Handle == 0)
                continue;

            SpriteRegistryEntry entry;
            entry.TextureHandle = importData.TextureHandle;
            entry.SpriteIndex = spriteIndex;
            m_SpriteRegistry[sprite.Handle] = entry;
        }
    }

    void AssetManager::UnregisterSpritesForTexture(AssetHandle textureHandle)
    {
        for (auto iterator = m_SpriteRegistry.begin(); iterator != m_SpriteRegistry.end();)
        {
            if (iterator->second.TextureHandle == textureHandle)
                iterator = m_SpriteRegistry.erase(iterator);
            else
                ++iterator;
        }
    }

    TextureImportData& AssetManager::GetOrCreateTextureImportData(AssetHandle textureHandle)
    {
        auto iterator = m_TextureImportData.find(textureHandle);
        if (iterator != m_TextureImportData.end())
            return iterator->second;

        EnsureDefaultTextureMeta(textureHandle);
        return m_TextureImportData.at(textureHandle);
    }

    const TextureImportData* AssetManager::GetTextureImportData(AssetHandle textureHandle) const
    {
        auto iterator = m_TextureImportData.find(textureHandle);
        if (iterator == m_TextureImportData.end())
            return nullptr;
        return &iterator->second;
    }

    bool AssetManager::LoadTextureImportData(AssetHandle textureHandle)
    {
        if (!IsAssetHandleValid(textureHandle))
            return false;

        const AssetMetadata& metadata = m_AssetRegistry.at(textureHandle);
        std::filesystem::path filesystemPath = Project::GetAssetFileSystemPath(metadata.FilePath);

        TextureImportData importData;
        if (TextureImportSerializer::Deserialize(filesystemPath, importData))
        {
            importData.TextureHandle = textureHandle;
            m_TextureImportData[textureHandle] = importData;
            RegisterSpritesFromImportData(importData);
            return true;
        }

        return false;
    }

    bool AssetManager::SaveTextureImportData(AssetHandle textureHandle)
    {
        auto iterator = m_TextureImportData.find(textureHandle);
        if (iterator == m_TextureImportData.end())
            return false;

        if (!IsAssetHandleValid(textureHandle))
            return false;

        const AssetMetadata& metadata = m_AssetRegistry.at(textureHandle);
        std::filesystem::path filesystemPath = Project::GetAssetFileSystemPath(metadata.FilePath);

        iterator->second.TextureHandle = textureHandle;
        RegisterSpritesFromImportData(iterator->second);
        return TextureImportSerializer::Serialize(filesystemPath, iterator->second);
    }

    void AssetManager::EnsureDefaultTextureMeta(AssetHandle textureHandle)
    {
        if (m_TextureImportData.find(textureHandle) != m_TextureImportData.end())
            return;

        if (LoadTextureImportData(textureHandle))
            return;

        if (!IsAssetHandleValid(textureHandle))
            return;

        const AssetMetadata& metadata = m_AssetRegistry.at(textureHandle);
        std::filesystem::path filesystemPath = Project::GetAssetFileSystemPath(metadata.FilePath);

        uint32_t textureWidth = 1;
        uint32_t textureHeight = 1;
        if (IsAssetLoaded(textureHandle))
        {
            Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(m_LoadedAssets.at(textureHandle));
            textureWidth = texture->GetWidth();
            textureHeight = texture->GetHeight();
        }
        else if (std::filesystem::exists(filesystemPath))
        {
            Ref<Texture2D> texture = Texture2D::Create(filesystemPath.string());
            if (texture)
            {
                textureWidth = texture->GetWidth();
                textureHeight = texture->GetHeight();
            }
        }

        TextureImportData importData = SpriteSheetUtility::CreateDefaultSingleSprite(
            textureHandle, textureWidth, textureHeight);
        m_TextureImportData[textureHandle] = importData;
        RegisterSpritesFromImportData(importData);
        TextureImportSerializer::Serialize(filesystemPath, importData);
    }

    void AssetManager::ApplyGridSliceToTexture(AssetHandle textureHandle)
    {
        TextureImportData& importData = GetOrCreateTextureImportData(textureHandle);

        uint32_t textureWidth = 1;
        uint32_t textureHeight = 1;
        Ref<Asset> asset = GetAsset(textureHandle);
        if (asset)
        {
            Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(asset);
            textureWidth = texture->GetWidth();
            textureHeight = texture->GetHeight();
        }

        SpriteSheetUtility::ApplyGridSlice(importData, textureWidth, textureHeight);
        SaveTextureImportData(textureHandle);
    }

    const SpriteDefinition* AssetManager::GetSpriteDefinition(AssetHandle spriteHandle) const
    {
        auto registryIterator = m_SpriteRegistry.find(spriteHandle);
        if (registryIterator == m_SpriteRegistry.end())
            return nullptr;

        const SpriteRegistryEntry& entry = registryIterator->second;
        auto importIterator = m_TextureImportData.find(entry.TextureHandle);
        if (importIterator == m_TextureImportData.end())
            return nullptr;

        if (entry.SpriteIndex >= importIterator->second.Sprites.size())
            return nullptr;

        return &importIterator->second.Sprites[entry.SpriteIndex];
    }

    SpriteResolved AssetManager::ResolveSprite(AssetHandle spriteHandle)
    {
        SpriteResolved resolved;
        auto registryIterator = m_SpriteRegistry.find(spriteHandle);
        if (registryIterator == m_SpriteRegistry.end())
            return resolved;

        return ResolveSpriteFromTexture(registryIterator->second.TextureHandle,
                                      registryIterator->second.SpriteIndex);
    }

    SpriteResolved AssetManager::ResolveSpriteFromTexture(AssetHandle textureHandle, uint32_t spriteIndex)
    {
        SpriteResolved resolved;

        if (!IsAssetHandleValid(textureHandle))
            return resolved;

        EnsureDefaultTextureMeta(textureHandle);

        const TextureImportData& importData = m_TextureImportData.at(textureHandle);
        if (spriteIndex >= importData.Sprites.size())
            return resolved;

        Ref<Asset> asset = GetAsset(textureHandle);
        if (!asset)
            return resolved;

        Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(asset);
        const SpriteDefinition& sprite = importData.Sprites[spriteIndex];

        resolved.Texture = texture;
        resolved.UVs = SpriteSheetUtility::PixelRectToWorldQuadUVs(
            sprite.PixelRect, texture->GetWidth(), texture->GetHeight());
        resolved.Pivot = sprite.Pivot;
        resolved.PixelSize = {sprite.PixelRect.z, sprite.PixelRect.w};
        resolved.PixelsPerUnit = importData.PixelsPerUnit > 0 ? importData.PixelsPerUnit : 100;
        resolved.IsValid = true;
        return resolved;
    }

    const std::vector<SpriteDefinition>& AssetManager::GetSpritesForTexture(AssetHandle textureHandle)
    {
        EnsureDefaultTextureMeta(textureHandle);
        return m_TextureImportData.at(textureHandle).Sprites;
    }

    AssetHandle AssetManager::GetDefaultSpriteHandleForTexture(AssetHandle textureHandle)
    {
        EnsureDefaultTextureMeta(textureHandle);
        const auto& sprites = m_TextureImportData.at(textureHandle).Sprites;
        if (sprites.empty())
            return 0;
        return sprites[0].Handle;
    }

    AssetHandle AssetManager::GetTextureHandleForSprite(AssetHandle spriteAssetHandle) const
    {
        auto registryIterator = m_SpriteRegistry.find(spriteAssetHandle);
        if (registryIterator == m_SpriteRegistry.end())
            return 0;
        return registryIterator->second.TextureHandle;
    }

} // namespace Himii
