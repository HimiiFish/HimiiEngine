#pragma once
#include "Hepch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Himii
{
    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
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

    Ref<VertexBuffer> VertexBuffer::Create(float *vertices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
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
    Ref<IndexBuffer> IndexBuffer::Create(uint32_t *indices, uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, size);
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
} // namespace Himii
