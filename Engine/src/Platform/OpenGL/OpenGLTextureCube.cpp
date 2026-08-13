#include "Hepch.h"

#include "OpenGLTextureCube.h"
#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/Log.h"
#include "stb_image.h"

namespace Himii
{
    namespace
    {
        GLenum CubemapImageFormatToInternal(ImageFormat format)
        {
            switch (format)
            {
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
            return GL_RGB8;
        }

        GLenum CubemapImageFormatToData(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::RGB8:
                case ImageFormat::RGB16F:
                    return GL_RGB;
                case ImageFormat::RGBA8:
                case ImageFormat::RGBA32F:
                    return GL_RGBA;
                case ImageFormat::RG16F:
                    return GL_RG;
                default:
                    break;
            }
            HIMII_CORE_ASSERT(false);
            return GL_RGB;
        }

        GLenum CubemapImageFormatToType(ImageFormat format)
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

        uint32_t CubemapImageFormatBytesPerPixel(ImageFormat format)
        {
            switch (format)
            {
                case ImageFormat::RGB8:
                    return 3;
                case ImageFormat::RGBA8:
                    return 4;
                case ImageFormat::RGBA32F:
                    return 16;
                case ImageFormat::RGB16F:
                    return 12; // uploaded as float RGB
                case ImageFormat::RG16F:
                    return 8; // uploaded as float RG
                default:
                    break;
            }
            HIMII_CORE_ASSERT(false);
            return 4;
        }

        uint32_t CalculateMipLevelCount(uint32_t size, bool generateMips)
        {
            if (!generateMips)
                return 1;
            uint32_t levels = 1;
            while ((1u << (levels - 1)) < size)
                ++levels;
            return levels;
        }
    }

    void OpenGLTextureCube::CreateEmptyStorage(const TextureSpecification &specification)
    {
        m_Specification = specification;
        m_Width = specification.Width;
        m_Height = specification.Height;
        m_InternalFormat = CubemapImageFormatToInternal(specification.Format);
        m_DataFormat = CubemapImageFormatToData(specification.Format);
        m_DataType = CubemapImageFormatToType(specification.Format);
        m_BytesPerPixel = CubemapImageFormatBytesPerPixel(specification.Format);
        m_MipLevelCount = CalculateMipLevelCount(m_Width, specification.GenerateMips);

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, static_cast<GLsizei>(m_MipLevelCount), m_InternalFormat,
                           static_cast<GLsizei>(m_Width), static_cast<GLsizei>(m_Height));

        const GLint magFilter = specification.UseLinearFiltering ? GL_LINEAR : GL_NEAREST;
        const GLint minFilter = specification.GenerateMips
                                        ? (specification.UseLinearFiltering ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST)
                                        : magFilter;
        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_RendererID, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(m_MipLevelCount - 1));
        m_IsLoaded = true;
    }

    OpenGLTextureCube::OpenGLTextureCube(const TextureSpecification &specification)
    {
        CreateEmptyStorage(specification);
    }

    OpenGLTextureCube::OpenGLTextureCube(const std::vector<std::string> &paths)
    {
        TextureSpecification specification;
        specification.Format = ImageFormat::RGB8;
        specification.ClampToEdge = true;
        specification.UseLinearFiltering = true;
        specification.GenerateMips = false;

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_set_flip_vertically_on_load(false);

        if (!paths.empty())
            m_Path = paths[0];

        std::vector<unsigned char *> facePointers(paths.size(), nullptr);
        for (size_t index = 0; index < paths.size(); ++index)
        {
            unsigned char *data = nullptr;
            const auto fileBytes = FileSystem::ReadBytes(paths[index]);
            if (fileBytes)
                data = stbi_load_from_memory(fileBytes->data(), static_cast<int>(fileBytes->size()), &width, &height,
                                             &channels, 3);
            else
                data = stbi_load(paths[index].c_str(), &width, &height, &channels, 3);

            if (!data)
            {
                HIMII_CORE_ERROR("Failed to load cubemap texture: {0}", paths[index]);
                continue;
            }
            facePointers[index] = data;
            specification.Width = static_cast<uint32_t>(width);
            specification.Height = static_cast<uint32_t>(height);
        }

        if (specification.Width == 0 || specification.Height == 0)
        {
            HIMII_CORE_ERROR("Cubemap creation failed: no valid faces");
            return;
        }

        CreateEmptyStorage(specification);

        for (uint32_t faceIndex = 0; faceIndex < facePointers.size() && faceIndex < 6; ++faceIndex)
        {
            if (!facePointers[faceIndex])
                continue;
            const uint32_t faceByteCount = m_Width * m_Height * m_BytesPerPixel;
            SetFaceData(faceIndex, 0, facePointers[faceIndex], faceByteCount);
            stbi_image_free(facePointers[faceIndex]);
        }
    }

    OpenGLTextureCube::~OpenGLTextureCube()
    {
        if (m_RendererID != 0)
            glDeleteTextures(1, &m_RendererID);
    }

    void OpenGLTextureCube::SetData(void *data, uint32_t size)
    {
        (void)data;
        (void)size;
        HIMII_CORE_ASSERT(false, "TextureCube::SetData is not supported; use SetFaceData");
    }

    void OpenGLTextureCube::SetDataRegion(
            uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data, uint32_t size)
    {
        (void)offsetX;
        (void)offsetY;
        (void)width;
        (void)height;
        (void)data;
        (void)size;
        HIMII_CORE_ASSERT(false, "TextureCube::SetDataRegion is not supported; use SetFaceData");
    }

    void OpenGLTextureCube::SetFaceData(uint32_t faceIndex, uint32_t mipLevel, const void *data, uint32_t size)
    {
        HIMII_CORE_ASSERT(faceIndex < 6);
        HIMII_CORE_ASSERT(mipLevel < m_MipLevelCount);
        const uint32_t mipWidth = std::max(1u, m_Width >> mipLevel);
        const uint32_t mipHeight = std::max(1u, m_Height >> mipLevel);
        HIMII_CORE_ASSERT(size == mipWidth * mipHeight * m_BytesPerPixel);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage3D(m_RendererID, static_cast<GLint>(mipLevel), 0, 0, static_cast<GLint>(faceIndex),
                            static_cast<GLsizei>(mipWidth), static_cast<GLsizei>(mipHeight), 1, m_DataFormat,
                            m_DataType, data);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }

    void OpenGLTextureCube::Bind(uint32_t slot) const
    {
        glBindTextureUnit(slot, m_RendererID);
    }
}
