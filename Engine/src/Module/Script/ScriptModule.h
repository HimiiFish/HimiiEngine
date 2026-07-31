#pragma once

#include "Module/IModule.h"
#include "Module/Script/ScriptCompiler.h"
#include "Module/Script/ScriptEngine.h"

namespace Himii
{
    /// Application 级脚本模块：封装 ScriptEngine / ScriptCompiler 生命周期。
    class ScriptModule : public IModule
    {
    public:
        const char *GetModuleName() const override { return "Script"; }

        void OnInitialize() override
        {
            ScriptEngine::Init();
        }

        void OnShutdown() override
        {
            ScriptCompiler::Shutdown();
            ScriptEngine::Shutdown();
        }
    };
}
