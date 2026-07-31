#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级粒子模块；注册时挂到 WorldUpdatePhase::Presentation。
    class ParticleModule : public IWorldModule
    {
    public:
        explicit ParticleModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "Particle"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
