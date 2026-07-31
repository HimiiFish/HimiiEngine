#include "Hepch.h"
#include "World/WorldModuleRegistry.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    void WorldModuleRegistry::RegisterModule(
            WorldUpdatePhase phase, Scope<IWorldModule> module)
    {
        if (!module)
            return;

        if (m_Initialized)
        {
            HIMII_CORE_ERROR(
                    "WorldModuleRegistry: cannot register '{0}' after InitializeAll.",
                    module->GetModuleName());
            return;
        }

        const size_t phaseIndex = static_cast<size_t>(phase);
        if (phaseIndex >= PhaseCount)
        {
            HIMII_CORE_ERROR(
                    "WorldModuleRegistry: invalid phase for module '{0}'.",
                    module->GetModuleName());
            return;
        }

        HIMII_CORE_INFO(
                "WorldModuleRegistry: registered '{0}' -> {1}",
                module->GetModuleName(),
                GetWorldUpdatePhaseName(phase));

        m_ModulesByPhase[phaseIndex].push_back(std::move(module));
    }

    void WorldModuleRegistry::InitializeAll()
    {
        if (m_Initialized)
            return;

        for (size_t phaseIndex = 0; phaseIndex < PhaseCount; ++phaseIndex)
        {
            for (Scope<IWorldModule> &module : m_ModulesByPhase[phaseIndex])
                module->OnInitialize();
        }

        m_Initialized = true;
    }

    void WorldModuleRegistry::RuntimeStartAll()
    {
        if (!m_Initialized)
            return;

        for (size_t phaseIndex = 0; phaseIndex < PhaseCount; ++phaseIndex)
        {
            for (Scope<IWorldModule> &module : m_ModulesByPhase[phaseIndex])
                module->OnRuntimeStart();
        }
    }

    void WorldModuleRegistry::RuntimeStopAll()
    {
        if (!m_Initialized)
            return;

        for (size_t phaseIndex = PhaseCount; phaseIndex > 0; --phaseIndex)
        {
            auto &bucket = m_ModulesByPhase[phaseIndex - 1];
            for (auto iterator = bucket.rbegin(); iterator != bucket.rend(); ++iterator)
                (*iterator)->OnRuntimeStop();
        }
    }

    void WorldModuleRegistry::ShutdownAll()
    {
        if (!m_Initialized)
        {
            for (auto &bucket : m_ModulesByPhase)
                bucket.clear();
            return;
        }

        for (size_t phaseIndex = PhaseCount; phaseIndex > 0; --phaseIndex)
        {
            auto &bucket = m_ModulesByPhase[phaseIndex - 1];
            for (auto iterator = bucket.rbegin(); iterator != bucket.rend(); ++iterator)
                (*iterator)->OnShutdown();
            bucket.clear();
        }

        m_Initialized = false;
    }

    void WorldModuleRegistry::Update(WorldUpdatePhase phase, Timestep timestep)
    {
        if (!m_Initialized)
            return;

        const size_t phaseIndex = static_cast<size_t>(phase);
        if (phaseIndex >= PhaseCount)
            return;

        for (Scope<IWorldModule> &module : m_ModulesByPhase[phaseIndex])
            module->OnUpdate(timestep);
    }

    void WorldModuleRegistry::UpdateAllPhasesInEnumOrder(Timestep timestep)
    {
        for (size_t phaseIndex = 0; phaseIndex < PhaseCount; ++phaseIndex)
            Update(static_cast<WorldUpdatePhase>(phaseIndex), timestep);
    }
}
