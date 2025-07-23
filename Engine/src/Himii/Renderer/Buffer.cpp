#include "Hepch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Himii/Core/Log.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Himii
{
    VertexBuffer *VertexBuffer::Create(float *vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::OpenGL:
                return new OpenGLVertexBuffer(vertices, size);
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
    IndexBuffer *IndexBuffer::Create(uint32_t *indices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::None:
                HIMII_CORE_ASSERT_F(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::OpenGL:
                return new OpenGLIndexBuffer(indices, size);
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
} // namespace Himii
