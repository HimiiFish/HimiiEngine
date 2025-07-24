#pragma once
#include "Himii/Renderer/RenderCommand.h"

namespace Himii
{
    class Renderer
    {
    public:
        static void BeginScene();
        static void EndScene();

        static void Submit(const Ref<VertexArray> &vertexArray);

        inline static RendererAPI::API GetAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}