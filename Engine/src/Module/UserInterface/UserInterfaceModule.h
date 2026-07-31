#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级 UI 输入模块；注册时挂到 WorldUpdatePhase::UserInterface（在脚本之前）。
    class UserInterfaceModule : public IWorldModule
    {
    public:
        explicit UserInterfaceModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "UserInterface"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
