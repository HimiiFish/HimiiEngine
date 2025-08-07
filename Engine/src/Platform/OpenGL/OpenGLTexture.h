#pragma once
#include "Himii/Renderer/Texture.h"

#include "glad/glad.h"

namespace Himii
{
	class OpenGLTexture:public Texture2D 
    {
        // Í¨¹ý Texture2D ¼Ì³Ð
    public:
        OpenGLTexture(uint32_t width, uint32_t height);
        OpenGLTexture(const std::string &path);
        virtual ~OpenGLTexture();

        virtual uint32_t GetWidth() const override
        {
            return m_Width;
        }
        virtual uint32_t GetHeight() const override
        {
            return m_Height;
        }

        virtual void SetData(void *data, uint32_t size) override;
        virtual void Bind(uint32_t slot=0) const override;
        virtual bool operator==(const Texture &other) const override
        {
            return m_RendererID==((OpenGLTexture&)other).m_RendererID;
        };

    private:
        std::string m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_RendererID;
        GLenum m_InternalFormat, m_DataFormat;
    };
}