#pragma once
#include "Module/Render/Texture.h"

#include "glad/glad.h"

namespace Himii
{
    class OpenGLTexture : public Texture2D {
    public:
        OpenGLTexture(uint32_t width, uint32_t height);
        explicit OpenGLTexture(const TextureSpecification &specification);
        OpenGLTexture(const std::string &path);
        virtual ~OpenGLTexture();

        virtual const TextureSpecification &GetSpecification() const override
        {
            return m_Specification;
        }

        virtual uint32_t GetWidth() const override
        {
            return m_Width;
        }
        virtual uint32_t GetHeight() const override
        {
            return m_Height;
        }
        virtual uint32_t GetRendererID() const override
        {
            return m_RendererID;
        }

        virtual const std::string &GetPath() const override
        {
            return m_Path;
        }
        virtual void SetData(void *data, uint32_t size) override;
        virtual void SetDataRegion(
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data,
                uint32_t size) override;
        virtual void Bind(uint32_t slot = 0) const override;
        virtual bool IsLoaded() const override
        {
            return m_IsLoaded;
        }
        virtual bool operator==(const Texture &other) const override
        {
            return m_RendererID == other.GetRendererID();
        };

    private:
        void CreateStorage(const TextureSpecification &specification);

        TextureSpecification m_Specification;

        std::string m_Path;
        bool m_IsLoaded = false;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_RendererID = 0;
        GLenum m_InternalFormat = GL_RGBA8;
        GLenum m_DataFormat = GL_RGBA;
        uint32_t m_BytesPerPixel = 4;
    };
}
