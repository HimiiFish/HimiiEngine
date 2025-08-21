#include "Hepch.h"
#include "Himii/Core/Log.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <array>

namespace Himii
{
    OpenGLTexture::OpenGLTexture(uint32_t width, uint32_t height) : m_Width(width), m_Height(height)
    {
        HIMII_PROFILE_FUNCTION();

         m_InternalFormat = GL_RGBA8;
         m_DataFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, m_InternalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    OpenGLTexture::OpenGLTexture(const std::string &path) : m_Path(path)
    {
        HIMII_PROFILE_FUNCTION();

        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        stbi_uc *data = nullptr;
        {
            HIMII_PROFILE_SCOPE("stbi_load - OpenGLTexture::OpenGLTexture(const std::string&)");
            data=stbi_load(path.c_str(), &width, &height, &channels, 0);
        }
        
        HIMII_CORE_ASSERT(data, "Failed to load image");
        m_Width=width;
        m_Height = height;

        GLenum internalFormat = 0,dataFormat=0;
        if (channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else if (channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }

        m_InternalFormat=internalFormat;
        m_DataFormat = dataFormat;

        HIMII_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

        glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
        glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

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

        uint32_t bpp= m_DataFormat == GL_RGBA ? 4:3;
        HIMII_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
        glTextureSubImage2D(m_RendererID, 0, 0, 0,m_Width,m_Height,m_DataFormat,GL_UNSIGNED_BYTE,data);
    }
    void OpenGLTexture::Bind(uint32_t slot) const
    {
        HIMII_PROFILE_FUNCTION();

        glBindTextureUnit(slot, m_RendererID);
    }

    std::array<glm::vec2,4> OpenGLTexture::GetUVFromGrid(int col, int row, int cols, int rows, float paddingNorm) const
    {
        // 归一 padding 转为 UV 偏移：按列/行均匀切分
        float du = 1.0f / (float)cols;
        float dv = 1.0f / (float)rows;
        float u0 = col * du + paddingNorm;
        float v0 = row * dv + paddingNorm;
        float u1 = (col + 1) * du - paddingNorm;
        float v1 = (row + 1) * dv - paddingNorm;
        return { glm::vec2{u0,v0}, glm::vec2{u1,v0}, glm::vec2{u1,v1}, glm::vec2{u0,v1} };
    }

    std::array<glm::vec2,4> OpenGLTexture::GetUVFromPixels(const glm::vec2& pixelMin,
                                                            const glm::vec2& pixelMax,
                                                            const glm::vec2& paddingPx) const
    {
        float u0 = (pixelMin.x + paddingPx.x) / (float)m_Width;
        float v0 = (pixelMin.y + paddingPx.y) / (float)m_Height;
        float u1 = (pixelMax.x - paddingPx.x) / (float)m_Width;
        float v1 = (pixelMax.y - paddingPx.y) / (float)m_Height;
        return { glm::vec2{u0,v0}, glm::vec2{u1,v0}, glm::vec2{u1,v1}, glm::vec2{u0,v1} };
    }
} // namespace Himii
