#pragma once
#include "Himii/Renderer/RendererAPI.h"

namespace Himii
{
    class RenderCommand 
    {
    public:
        inline static void Init()
        {
            s_RendererAPI->Init();
        }

        inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RendererAPI->SetViewport(x, y, width, height);
        }

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
        //inline static Scope<RendererAPI> s_RendererAPI = RendererAPI::Create();
        static Scope<RendererAPI> s_RendererAPI;
    };

} // namespace Himii
