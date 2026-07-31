#pragma once

#include "EngineCore/Core/Timestep.h"

namespace Himii
{
    /// World 级模块入口。所属阶段在注册时用 WorldUpdatePhase 显式指定，不在此用数字排序。
    class IWorldModule
    {
    public:
        virtual ~IWorldModule() = default;

        virtual const char *GetModuleName() const = 0;

        virtual void OnInitialize() = 0;
        virtual void OnShutdown() = 0;

        /// Play / Simulate 开始与结束（可选）。
        virtual void OnRuntimeStart() {}
        virtual void OnRuntimeStop() {}

        virtual void OnUpdate(Timestep timestep) = 0;
    };
}
