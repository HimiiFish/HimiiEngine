#include "Himii/Renderer/VertexArray.h"
#include "Himii/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Himii
{
    VertexArray* VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::OpenGL:
                return new OpenGLVertexArray();
            case RendererAPI::Vulkan:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::Vulkan is currently not supported!");
                return nullptr;
            case RendererAPI::DirectX12:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::DirectX12 is currently not supported!");
                return nullptr;
            case RendererAPI::Metal:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::Metal is currently not supported!");
                return nullptr;
        }
        HIMII_CORE_ASSERT_F(false, "Unknown RendererAPI!");
        return nullptr;
    }
}