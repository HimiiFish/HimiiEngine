#pragma once

namespace Himii
{
    enum class RendererAPI{
        None = 0,
        OpenGL,
        Vulkan,
        DirectX12,
        Metal
    };

    class Renderer
    {
    public:
        inline static RendererAPI GetAPI()
        {
            return s_RendererAPI;
        }

        static RendererAPI s_RendererAPI;
    };
}