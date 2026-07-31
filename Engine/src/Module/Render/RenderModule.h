#pragma once

#include "Module/IModule.h"
#include "Module/Render/Renderer.h"
#include "Module/Render/Renderer2D.h"
#include "Module/Render/Renderer3D.h"

namespace Himii
{
    /// Application 级渲染模块：封装 Renderer 生命周期。
    class RenderModule : public IModule
    {
    public:
        const char *GetModuleName() const override { return "Render"; }

        void OnInitialize() override
        {
            Renderer::Init();
        }

        void OnShutdown() override
        {
            Renderer2D::Shutdown();
            Renderer3D::Shutdown();
        }

        static void OnWindowResize(uint32_t width, uint32_t height)
        {
            Renderer::OnWindowResize(width, height);
        }
    };
}
