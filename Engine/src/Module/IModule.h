#pragma once

#include "EngineCore/Core/Timestep.h"

namespace Himii
{
    /// Application 级模块入口。具体子系统继承并注册到 ModuleRegistry。
    class IModule
    {
    public:
        virtual ~IModule() = default;

        virtual const char *GetModuleName() const = 0;
        virtual void OnInitialize() = 0;
        virtual void OnShutdown() = 0;

        /// 可选：程序级每帧更新（默认空）。
        virtual void OnUpdate(Timestep timestep)
        {
            (void)timestep;
        }
    };
}
