#include "Hepch.h"
#include "Resource/AssetSerializerRegistry.h"
#include "EngineCore/Core/Log.h"

#include <algorithm>
#include <unordered_map>

namespace Himii
{
    namespace
    {
        std::unordered_map<AssetType, Scope<IAssetSerializer>> &GetSerializersByType()
        {
            static std::unordered_map<AssetType, Scope<IAssetSerializer>> serializers;
            return serializers;
        }

        std::unordered_map<std::string, AssetType> &GetExtensionToAssetType()
        {
            static std::unordered_map<std::string, AssetType> extensionMap;
            return extensionMap;
        }

        std::string NormalizeExtension(const std::string &extension)
        {
            std::string extensionLower = extension;
            std::transform(extensionLower.begin(), extensionLower.end(), extensionLower.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            return extensionLower;
        }
    }

    void AssetSerializerRegistry::Register(Scope<IAssetSerializer> serializer)
    {
        if (!serializer)
            return;
        const AssetType type = serializer->GetAssetType();
        GetSerializersByType()[type] = std::move(serializer);
    }

    void AssetSerializerRegistry::RegisterExtension(const std::string &extensionWithDot, AssetType type)
    {
        GetExtensionToAssetType()[NormalizeExtension(extensionWithDot)] = type;
    }

    void AssetSerializerRegistry::Clear()
    {
        GetSerializersByType().clear();
        GetExtensionToAssetType().clear();
    }

    bool AssetSerializerRegistry::HasSerializer(AssetType type)
    {
        return GetSerializersByType().find(type) != GetSerializersByType().end();
    }

    Ref<Asset> AssetSerializerRegistry::Deserialize(AssetType type, const std::filesystem::path &filepath)
    {
        auto iterator = GetSerializersByType().find(type);
        if (iterator == GetSerializersByType().end() || !iterator->second)
        {
            HIMII_CORE_ERROR("No asset serializer registered for type {0}",
                             Asset::AssetTypeToString(type));
            return nullptr;
        }
        return iterator->second->Deserialize(filepath);
    }

    AssetType AssetSerializerRegistry::GetAssetTypeFromExtension(const std::string &extension)
    {
        const auto &extensionMap = GetExtensionToAssetType();
        const auto iterator = extensionMap.find(NormalizeExtension(extension));
        if (iterator == extensionMap.end())
            return AssetType::None;
        return iterator->second;
    }
}
