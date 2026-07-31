#include "Hepch.h"
#include "Module/Render/RHI/RHI.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/OpenGL/OpenGLTextureCube.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include "GLFW/glfw3.h"

namespace Himii
{
    RHI::API RHI::s_API = RHI::API::OpenGL;

    Scope<RHI> RHI::Create()
    {
        switch (s_API)
        {
            case API::None:
                HIMII_CORE_ASSERT(false, "RHI::None is currently not supported!");
                return nullptr;
            case API::OpenGL:
                return CreateScope<OpenGLRendererAPI>();
            case API::Vulkan:
            case API::DirectX12:
            case API::Metal:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }

        HIMII_CORE_ASSERT(false, "Unknown RHI!");
        return nullptr;
    }

    Ref<VertexBuffer> RHI::CreateVertexBuffer(uint32_t size)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> RHI::CreateVertexBuffer(float *vertices, uint32_t size)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<IndexBuffer> RHI::CreateIndexBuffer(uint32_t *indices, uint32_t count)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, count);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<VertexArray> RHI::CreateVertexArray()
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLVertexArray>();
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<Texture2D> RHI::CreateTexture2D(const TextureSpecification &specification)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLTexture>(specification);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<Texture2D> RHI::CreateTexture2D(const std::string &path)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLTexture>(path);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<TextureCube> RHI::CreateTextureCube(const std::vector<std::string> &paths)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLTextureCube>(paths);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<Shader> RHI::CreateShader(const std::string &filepath)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLShader>(filepath);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<Shader> RHI::CreateShader(
            const std::string &name, const std::string &vertexSource, const std::string &fragmentSource)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLShader>(name, vertexSource, fragmentSource);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<Framebuffer> RHI::CreateFramebuffer(const FramebufferSpecification &specification)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLFramebuffer>(specification);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Ref<UniformBuffer> RHI::CreateUniformBuffer(uint32_t size, uint32_t binding)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateRef<OpenGLUniformBuffer>(size, binding);
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }

    Scope<GraphicsContext> RHI::CreateGraphicsContext(void *nativeWindowHandle)
    {
        switch (s_API)
        {
            case API::OpenGL:
                return CreateScope<OpenGLContext>(static_cast<GLFWwindow *>(nativeWindowHandle));
            default:
                HIMII_CORE_ASSERT(false, "Selected RHI backend is currently not supported!");
                return nullptr;
        }
    }
}
