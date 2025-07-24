#pragma once
#include "Himii/Renderer/RendererAPI.h"

namespace Himii
{
    class RenderCommand 
    {
    public:
        inline static void SetClearColor(const glm::vec4& color)
        {
            s_RendererAPI->SetClearColor(color);
        }
        inline static void Clear()
        {
            s_RendererAPI->Clear();
        }

        inline static void DrawIndexed(const Ref<VertexArray> &vertexArray)
        {
            s_RendererAPI->DrawIndexed(vertexArray);
        }
    private:
        inline static Scope<RendererAPI> s_RendererAPI = RendererAPI::Create();
    };

} // namespace Himii
