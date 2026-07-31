#include "Hepch.h"
#include "Module/ModuleRegistry.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    void ModuleRegistry::RegisterModule(Scope<IModule> module)
    {
        if (!module)
            return;

        if (m_Initialized)
        {
            HIMII_CORE_ERROR(
                    "ModuleRegistry: cannot register '{0}' after InitializeAll.",
                    module->GetModuleName());
            return;
        }

        HIMII_CORE_INFO("ModuleRegistry: registered '{0}'", module->GetModuleName());
        m_Modules.push_back(std::move(module));
    }

    void ModuleRegistry::InitializeAll()
    {
        if (m_Initialized)
            return;

        for (Scope<IModule> &module : m_Modules)
            module->OnInitialize();

        m_Initialized = true;
    }

    void ModuleRegistry::ShutdownAll()
    {
        if (!m_Initialized)
        {
            m_Modules.clear();
            return;
        }

        for (auto iterator = m_Modules.rbegin(); iterator != m_Modules.rend(); ++iterator)
            (*iterator)->OnShutdown();

        m_Modules.clear();
        m_Initialized = false;
    }

    void ModuleRegistry::UpdateAll(Timestep timestep)
    {
        if (!m_Initialized)
            return;

        for (Scope<IModule> &module : m_Modules)
            module->OnUpdate(timestep);
    }
}
