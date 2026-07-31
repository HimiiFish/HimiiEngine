#pragma once

#include "Module/IModule.h"
#include "Module/Audio/AudioEngine.h"

namespace Himii
{
    /// Application 级音频模块：封装 AudioEngine 生命周期。
    class AudioModule : public IModule
    {
    public:
        const char *GetModuleName() const override { return "Audio"; }

        void OnInitialize() override
        {
            AudioEngine::Init();
        }

        void OnShutdown() override
        {
            AudioEngine::Shutdown();
        }
    };
}
