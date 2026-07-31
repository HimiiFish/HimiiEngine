#include "Hepch.h"
#include "Module/Render/Texture.h"
#include "Module/Render/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/OpenGL/OpenGLTextureCube.h"

namespace Himii
{
    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        TextureSpecification specification;
        specification.Width = width;
        specification.Height = height;
        specification.Format = ImageFormat::RGBA8;
        return Create(specification);
    }

    Ref<Texture2D> Texture2D::Create(const TextureSpecification &specification)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTexture>(specification);
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

    Ref<Texture2D> Texture2D::Create(const std::string &path)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTexture>(path);
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

    Ref<TextureCube> TextureCube::Create(const std::vector<std::string> &path)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLTextureCube>(path);
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
