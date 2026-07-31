#pragma once

#include "World/IWorldModule.h"
#include "Module/Physics/Physics2DWorld.h"

namespace Himii
{
    class Scene;

    /// World 级 2D 物理模块；注册时挂到 WorldUpdatePhase::Physics。
    class Physics2DModule : public IWorldModule
    {
    public:
        explicit Physics2DModule(Scene *scene);

        const char *GetModuleName() const override { return "Physics2D"; }

        void OnInitialize() override;
        void OnShutdown() override;

        void OnRuntimeStart() override;
        void OnRuntimeStop() override;
        void OnUpdate(Timestep timestep) override;

        Physics2DWorld &GetPhysics2DWorld() { return m_Physics2DWorld; }
        const Physics2DWorld &GetPhysics2DWorld() const { return m_Physics2DWorld; }

    private:
        Scene *m_Scene = nullptr;
        Physics2DWorld m_Physics2DWorld;
    };
}
