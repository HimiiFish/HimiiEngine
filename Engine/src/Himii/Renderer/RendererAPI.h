#pragma once
#include "Himii/Core/Core.h"
#include "Himii/Renderer/VertexArray.h"
#include "glm/glm.hpp"

namespace Himii
{
    class RendererAPI {
    public:
        enum class API {
            None = 0,
            OpenGL,
            Vulkan,
            DirectX12,
            Metal
        };

    public:
        virtual void Init() = 0;
        virtual void SetClearColor(const glm::vec4 &color) = 0;
        virtual void Clear() = 0;

        virtual void DrawIndexed(const Ref<VertexArray> &vertexArray) = 0;
        static API GetAPI()
        {
            return s_API;
        }
        static Scope<RendererAPI> Create();

    private:
        static API s_API;
    };
} // namespace Himii
