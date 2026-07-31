#pragma once
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    class RenderCommand
    {
    public:
        inline static void Init() { s_RHI->Init(); }

        inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RHI->SetViewport(x, y, width, height);
        }

        inline static void SetClearColor(const glm::vec4 &color) { s_RHI->SetClearColor(color); }

        inline static void Clear() { s_RHI->Clear(); }

        inline static void DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0)
        {
            s_RHI->DrawIndexed(vertexArray, indexCount);
        }

        inline static void DrawIndexedInstanced(
                const Ref<VertexArray> &vertexArray, uint32_t indexCount, uint32_t instanceCount)
        {
            s_RHI->DrawIndexedInstanced(vertexArray, indexCount, instanceCount);
        }

        inline static void DrawArrays(const Ref<VertexArray> &vertexArray, uint32_t vertexCount = 0)
        {
            s_RHI->DrawArrays(vertexArray, vertexCount);
        }

        inline static void DrawLines(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0)
        {
            s_RHI->DrawLines(vertexArray, indexCount);
        }

        inline static void SetLineWidth(float width) { s_RHI->SetLineWidth(width); }

        inline static void SetDepthTest(bool enabled) { s_RHI->SetDepthTest(enabled); }

        inline static void SetDepthMask(bool enabled) { s_RHI->SetDepthMask(enabled); }

        inline static void SetDepthFunc(RHI::DepthComp function) { s_RHI->SetDepthFunc(function); }

        inline static void SetCullMode(RHI::CullMode mode) { s_RHI->SetCullMode(mode); }

    private:
        static Scope<RHI> s_RHI;
    };
}
