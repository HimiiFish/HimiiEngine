#include "Hepch.h"
#include "Himii/Renderer/VertexArray.h"
#include "Himii/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Himii
{
    VertexArray* VertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return new OpenGLVertexArray();
            case RendererAPI::API::Vulkan:
                HIMII_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
                return nullptr;
            case RendererAPI::API::DirectX12:
                HIMII_CORE_ASSERT(false, "RendererAPI::DirectX12 is currently not supported!");
                return nullptr;
            case RendererAPI::API::Metal:
                HIMII_CORE_ASSERT(false, "RendererAPI::Metal is currently not supported!");
                return nullptr;
        }
        HIMII_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
}