#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级脚本 FixedUpdate；注册时挂到 WorldUpdatePhase::ScriptFixedUpdate。
    class ScriptFixedUpdateModule : public IWorldModule
    {
    public:
        explicit ScriptFixedUpdateModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "ScriptFixedUpdate"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
