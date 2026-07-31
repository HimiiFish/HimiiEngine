#pragma once

#include "Module/IModule.h"
#include "EngineCore/Core/Core.h"

#include <vector>

namespace Himii
{
    /// Application 级模块注册表。注册顺序不等于更新顺序；当前按注册顺序 Update。
    class ModuleRegistry
    {
    public:
        void RegisterModule(Scope<IModule> module);
        void InitializeAll();
        void ShutdownAll();
        void UpdateAll(Timestep timestep);

        template<typename ModuleType>
        ModuleType *FindModule()
        {
            for (Scope<IModule> &module : m_Modules)
            {
                if (ModuleType *typed = dynamic_cast<ModuleType *>(module.get()))
                    return typed;
            }
            return nullptr;
        }

        bool IsInitialized() const { return m_Initialized; }

    private:
        std::vector<Scope<IModule>> m_Modules;
        bool m_Initialized = false;
    };
}
