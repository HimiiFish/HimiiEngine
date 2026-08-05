#pragma once
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    class OpenGLRendererAPI : public RHI
    {
    public:
        virtual void Init() override;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

        virtual void SetClearColor(const glm::vec4 &color) override;
        virtual void Clear() override;
        virtual void ClearDepth() override;

        virtual void DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0) override;
        virtual void DrawIndexedInstanced(
                const Ref<VertexArray> &vertexArray, uint32_t indexCount, uint32_t instanceCount) override;
        virtual void DrawArrays(const Ref<VertexArray> &vertexArray, uint32_t vertexCount) override;
        virtual void DrawLines(const Ref<VertexArray> &vertexArray, uint32_t vertexCount = 0) override;

        virtual void SetLineWidth(float width) override;
        virtual void SetDepthTest(bool enabled) override;
        virtual void SetDepthMask(bool enabled) override;
        virtual void SetDepthFunc(RHI::DepthComp function) override;
        virtual void SetCullMode(RHI::CullMode mode) override;
    };
}
