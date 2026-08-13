#include "Hepch.h"
#include "EngineCore/Core/Log.h"
#include "EngineCore/Core/FileSystem.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace Himii
{
    namespace Utils
    {
        static GLenum ImageFormatToGLDataFormat(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::R8:
                    return GL_RED;
                case ImageFormat::RGB8:
                    return GL_RGB;
                case ImageFormat::RGBA8:
                    return GL_RGBA;
                case ImageFormat::RGBA32F:
                    return GL_RGBA;
                case ImageFormat::RGB16F:
                    return GL_RGB;
                case ImageFormat::RG16F:
                    return GL_RG;
                default:
                    break;
            }

            HIMII_CORE_ASSERT(false);
            return 0;
        }

        static GLenum ImageFormatToGLInternalFormat(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::R8:
                    return GL_R8;
                case ImageFormat::RGB8:
                    return GL_RGB8;
                case ImageFormat::RGBA8:
                    return GL_RGBA8;
                case ImageFormat::RGBA32F:
                    return GL_RGBA32F;
                case ImageFormat::RGB16F:
                    return GL_RGB16F;
                case ImageFormat::RG16F:
                    return GL_RG16F;
                default:
                    break;
            }

            HIMII_CORE_ASSERT(false);
            return 0;
        }

        static GLenum ImageFormatToGLDataType(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::RGBA32F:
                case ImageFormat::RGB16F:
                case ImageFormat::RG16F:
                    return GL_FLOAT;
                default:
                    return GL_UNSIGNED_BYTE;
            }
        }

        static uint32_t ImageFormatBytesPerPixel(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::R8:
                    return 1;
                case ImageFormat::RGB8:
                    return 3;
                case ImageFormat::RGBA8:
                    return 4;
                case ImageFormat::RGBA32F:
                    return 16;
                case ImageFormat::RGB16F:
                    return 12;
                case ImageFormat::RG16F:
                    return 8;
                default:
                    break;
            }
            HIMII_CORE_ASSERT(false);
            return 4;
        }
    } // namespace Utils

    void OpenGLTexture::CreateStorage(const TextureSpecification &specification)
    {
        m_Specification = specification;
        m_Width = specification.Width;
        m_Height = specification.Height;
        m_InternalFormat = Utils::ImageFormatToGLInternalFormat(specification.Format);
        m_DataFormat = Utils::ImageFormatToGLDataFormat(specification.Format);
        m_BytesPerPixel = Utils::ImageFormatBytesPerPixel(specification.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

        const GLint filter = specification.UseLinearFiltering ? GL_LINEAR : GL_NEAREST;
        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, filter);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, filter);

        const GLint wrap = specification.ClampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, wrap);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, wrap);

        m_IsLoaded = true;
    }

    OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height)
    {
        HIMII_PROFILE_FUNCTION();

        TextureSpecification specification;
        specification.Width = width;
        specification.Height = height;
        specification.Format = ImageFormat::RGBA8;
        specification.ClampToEdge = true;
        specification.UseLinearFiltering = true;
        CreateStorage(specification);
    }

    OpenGLTexture::OpenGLTexture(const TextureSpecification &specification)
    {
        HIMII_PROFILE_FUNCTION();
        CreateStorage(specification);
    }

    OpenGLTexture::OpenGLTexture(const std::string &path) : m_Path(path)
    {
        HIMII_PROFILE_FUNCTION();

        int width = 0;
        int height = 0;
        int sourceChannels = 0;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc *data = nullptr;
        {
            HIMII_PROFILE_SCOPE("stbi_load - OpenGLTexture2D::OpenGLTexture2D(const std::string&)");
            const auto fileBytes = FileSystem::ReadBytes(path);
            if (fileBytes)
                data = stbi_load_from_memory(fileBytes->data(), static_cast<int>(fileBytes->size()),
                                             &width, &height, &sourceChannels, 4);
            else
                data = stbi_load(path.c_str(), &width, &height, &sourceChannels, 4);
        }
        HIMII_CORE_ASSERT(data, "Failed to load image!");

        TextureSpecification specification;
        specification.Width = static_cast<uint32_t>(width);
        specification.Height = static_cast<uint32_t>(height);
        specification.Format = ImageFormat::RGBA8;
        specification.ClampToEdge = false;
        specification.UseLinearFiltering = false;
        CreateStorage(specification);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE,
                            data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        stbi_image_free(data);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        HIMII_PROFILE_FUNCTION();
        glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTexture::SetData(void *data, uint32_t size)
    {
        HIMII_PROFILE_FUNCTION();
        HIMII_CORE_ASSERT(size == m_Width * m_Height * m_BytesPerPixel, "Data must be entire texture!");
        SetDataRegion(0, 0, m_Width, m_Height, data, size);
    }

    void OpenGLTexture::SetDataRegion(
            uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data,
            uint32_t size)
    {
        HIMII_PROFILE_FUNCTION();
        HIMII_CORE_ASSERT(offsetX + width <= m_Width && offsetY + height <= m_Height,
                          "Texture region out of bounds!");
        HIMII_CORE_ASSERT(size == width * height * m_BytesPerPixel, "Region data size mismatch!");

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage2D(
                m_RendererID, 0, static_cast<GLint>(offsetX), static_cast<GLint>(offsetY),
                static_cast<GLsizei>(width), static_cast<GLsizei>(height), m_DataFormat,
                Utils::ImageFormatToGLDataType(m_Specification.Format), data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

    void OpenGLTexture::Bind(uint32_t slot) const
    {
        HIMII_PROFILE_FUNCTION();
        glBindTextureUnit(slot, m_RendererID);
    }
} // namespace Himii
