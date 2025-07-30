#pragma once
#include "Himii/Renderer/Texture.h"

namespace Himii
{
	class OpenGLTexture:public Texture2D 
    {
        // Í¨¹ý Texture2D ¼Ì³Ð
    public:
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
        virtual void Bind(uint32_t slot=0) const override;

    private:
        std::string m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_RendererID;
    };
}