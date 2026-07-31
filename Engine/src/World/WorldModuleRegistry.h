#pragma once

#include "World/IWorldModule.h"
#include "World/WorldUpdatePhase.h"
#include "EngineCore/Core/Core.h"

#include <array>
#include <vector>

namespace Himii
{
    /// World 级模块注册表。
    /// 更新顺序由调用方通过 Update(WorldUpdatePhase) 显式指定；不使用注册序号或 priority 数字。
    class WorldModuleRegistry
    {
    public:
        /// 将模块挂到指定阶段桶（枚举显式传参）。
        void RegisterModule(WorldUpdatePhase phase, Scope<IWorldModule> module);

        void InitializeAll();
        void ShutdownAll();
        void RuntimeStartAll();
        void RuntimeStopAll();

        /// 显式执行某一阶段下的全部模块（同阶段内按注册先后，仅作同阶段稳定次序）。
        void Update(WorldUpdatePhase phase, Timestep timestep);

        /// 按 WorldUpdatePhase 枚举定义顺序依次 Update（仍由枚举驱动，非 priority）。
        void UpdateAllPhasesInEnumOrder(Timestep timestep);

        bool IsInitialized() const { return m_Initialized; }

    private:
        static constexpr size_t PhaseCount = static_cast<size_t>(WorldUpdatePhase::Count);

        std::array<std::vector<Scope<IWorldModule>>, PhaseCount> m_ModulesByPhase;
        bool m_Initialized = false;
    };
}
