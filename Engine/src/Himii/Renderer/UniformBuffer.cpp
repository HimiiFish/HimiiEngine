#include "Himii/Renderer/UniformBuffer.h"
#include "Himii/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"

namespace Himii
{
    Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLUniformBuffer>(size, binding);
        }

        HIMII_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }
} // namespace Himii
