#pragma once

#include "World/IWorldModule.h"

namespace Himii
{
    class Scene;

    /// World 级场景绘制模块；注册时挂到 WorldUpdatePhase::Render。
    /// 实际绘制参数由 World 在本阶段前写入 pending 请求。
    class SceneRenderModule : public IWorldModule
    {
    public:
        explicit SceneRenderModule(Scene *scene) : m_Scene(scene) {}

        const char *GetModuleName() const override { return "SceneRender"; }

        void OnInitialize() override {}
        void OnShutdown() override {}

        void OnUpdate(Timestep timestep) override;

    private:
        Scene *m_Scene = nullptr;
    };
}
