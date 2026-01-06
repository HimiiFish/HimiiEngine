#include "Himii/Asset/AssetManager.h"
#include "Himii/Core/Log.h"
#include "Himii/Project/Project.h"
#include "Himii/Renderer/Texture.h"
#include "Himii/Asset/AssetSerializer.h"
#include "yaml-cpp/yaml.h"
#include <fstream>

namespace Himii
{

    AssetManager::AssetManager()
    {
    }

    Ref<Asset> AssetManager::GetAsset(AssetHandle handle)
    {
        // 1. 检查是否已经加载
        if (IsAssetLoaded(handle))
            return m_LoadedAssets.at(handle);

        // 2. 检查是否在注册表中
        const auto &metadata = m_AssetRegistry[handle];
        if (!metadata) // 无效的 Metadata
            return nullptr;

        Ref<Asset> asset = nullptr;

        // 3. 根据类型加载资源
        // 注意：Project::GetAssetFileSystemPath 需要 Project.h
        std::filesystem::path filesystemPath = Project::GetAssetFileSystemPath(metadata.FilePath);
        std::string pathString = filesystemPath.string();

        switch (metadata.Type)
        {
            case AssetType::Texture2D:
            {
                // 加载 Texture2D，Texture2D::Create 应该返回 Ref<Texture2D>
                asset = Texture2D::Create(pathString);
                break;
            }
            case AssetType::SpriteAnimation:
            {
                asset = SpriteAnimationSerializer::Deserialize(pathString);
                break;
            }
            case AssetType::None:
            default:
                break;
        }

        // 4. 如果加载成功，存入缓存并返回
        if (asset)
        {
            asset->Handle = handle; // 确保内存中的 Asset 知道它自己的 Handle
            m_LoadedAssets[handle] = asset;
            m_AssetRegistry[handle].IsLoaded = true; // 标记元数据
        }

        return asset;
    }

    AssetHandle AssetManager::ImportAsset(const std::filesystem::path &filepath)
    {
        // 计算相对路径
        std::filesystem::path relativePath = filepath;

        // 检查该路径是否已经存在于 Registry 中 (防止重复导入)
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
            return metadata.Handle;
        }

        return 0; // Import Failed
    }

    // ... (keep invalid check methods same, or just replace whole block) ...
    // Skipping IsAssetHandleValid/IsAssetLoaded/GetAssetTypeFromExtension/SerializeAssetRegistry changes for brevity unless wanted.
    // I will replace from ImportAsset down to end for simplicity.

    bool AssetManager::IsAssetHandleValid(AssetHandle handle) const
    {
        return handle != 0 && m_AssetRegistry.find(handle) != m_AssetRegistry.end();
    }

    bool AssetManager::IsAssetLoaded(AssetHandle handle) const
    {
        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    AssetType AssetManager::GetAssetTypeFromExtension(const std::string &extension)
    {
        std::string ext = extension;
        // 转换为小写
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
            return AssetType::Texture2D;
        if (ext == ".anim")
            return AssetType::SpriteAnimation;
        if (ext == ".himii")
            return AssetType::Scene;

        return AssetType::None;
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

        std::ofstream fout(path);
        fout << out.c_str();
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
        catch (std::exception &e)
        {
            HIMII_CORE_ERROR("Failed to load asset registry: {0}", e.what());
            return false;
        }

        auto registryNode = data["AssetRegistry"];
        if (!registryNode)
        {
            HIMII_CORE_WARNING("AssetRegistry node missing in yaml!");
            return false;
        }

        for (auto node: registryNode)
        {
            AssetHandle handle = node["Handle"].as<uint64_t>();
            auto &metadata = m_AssetRegistry[handle];
            metadata.Handle = handle;
            metadata.FilePath = node["FilePath"].as<std::string>();
            metadata.Type = Asset::AssetTypeFromString(node["Type"].as<std::string>());
        }

        HIMII_CORE_INFO("Loaded AssetRegistry. Total assets: {0}", m_AssetRegistry.size());
        return true;
    }
} // namespace Himii
