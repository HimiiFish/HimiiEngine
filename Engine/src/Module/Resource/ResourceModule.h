#pragma once

#include "Module/IModule.h"
#include "Resource/AssetSerializerRegistry.h"
#include "Resource/ResourceSystem.h"

namespace Himii
{
    /// Application 级 Resource 模块：注册内置序列化器；Shutdown 时 Clear + Unbind。
    class ResourceModule : public IModule
    {
    public:
        const char *GetModuleName() const override { return "Resource"; }

        void OnInitialize() override
        {
            RegisterBuiltinAssetSerializers();
        }

        void OnShutdown() override
        {
            AssetSerializerRegistry::Clear();
            ResourceSystem::Unbind();
        }
    };
}
