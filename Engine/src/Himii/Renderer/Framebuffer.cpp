#include "Hepch.h"
#include "Himii/Renderer/Framebuffer.h"
#include "Himii/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"

namespace Himii {
    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification &spec)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::OpenGL: return CreateRef<OpenGLFramebuffer>(spec);
            default: HIMII_CORE_ASSERT(false, "Unsupported Renderer API for Framebuffer"); return nullptr;
        }
    }
}
