#pragma once
#include "EngineCore/Core/Core.h"
#include "Module/Render/VertexArray.h"
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
        enum class DepthComp { Never, Less, Equal, Lequal, Greater, Notequal, Gequal, Always };
        enum class CullMode { Front, Back, FrontAndBack, None };

        virtual void Init() = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetClearColor(const glm::vec4 &color) = 0;
        virtual void Clear() = 0;

        virtual void DrawIndexed(const Ref<VertexArray> &vertexArray,uint32_t indexCount=0) = 0;
        virtual void DrawIndexedInstanced(const Ref<VertexArray> &vertexArray, uint32_t indexCount, uint32_t instanceCount) = 0;
        virtual void DrawArrays(const Ref<VertexArray> &vertexArray, uint32_t vertexCount=0) = 0;
        virtual void DrawLines(const Ref<VertexArray> &vertexArray, uint32_t indexCount = 0) =0;
        virtual void SetLineWidth(float width)=0;
        virtual void SetDepthTest(bool enabled) = 0;
        virtual void SetDepthMask(bool enabled) = 0;
        virtual void SetDepthFunc(DepthComp func) = 0;
        virtual void SetCullMode(CullMode mode) = 0;
        static API GetAPI()
        {
            return s_API;
        }
        static Scope<RendererAPI> Create();

    private:
        static API s_API;
    };
} // namespace Himii
