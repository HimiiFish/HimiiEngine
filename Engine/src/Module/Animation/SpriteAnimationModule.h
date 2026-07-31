#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级精灵动画推进；注册时挂到 WorldUpdatePhase::Animation。
    class SpriteAnimationModule : public IWorldModule
    {
    public:
        explicit SpriteAnimationModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "SpriteAnimation"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
