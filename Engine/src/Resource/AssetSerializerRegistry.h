#pragma once

#include "Resource/IAssetSerializer.h"

#include <string>

namespace Himii
{
    /// 按 AssetType / 扩展名分发资产加载；领域实现由 Module 注册。
    class AssetSerializerRegistry
    {
    public:
        static void Register(Scope<IAssetSerializer> serializer);
        static void RegisterExtension(const std::string &extensionWithDot, AssetType type);
        static void Clear();

        static bool HasSerializer(AssetType type);
        static Ref<Asset> Deserialize(AssetType type, const std::filesystem::path &filepath);
        static AssetType GetAssetTypeFromExtension(const std::string &extension);
    };

    /// 注册内置扩展名与领域序列化器（由 ResourceModule::OnInitialize 调用）。
    void RegisterBuiltinAssetSerializers();
}
