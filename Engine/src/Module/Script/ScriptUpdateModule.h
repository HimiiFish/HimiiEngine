#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级脚本 Update；注册时挂到 WorldUpdatePhase::ScriptUpdate。
    class ScriptUpdateModule : public IWorldModule
    {
    public:
        explicit ScriptUpdateModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "ScriptUpdate"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
