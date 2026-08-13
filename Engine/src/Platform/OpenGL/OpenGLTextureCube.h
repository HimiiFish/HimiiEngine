#pragma once
#include "Module/Render/RenderCore/Texture.h"
#include "glad/glad.h"

namespace Himii
{
    class OpenGLTextureCube : public TextureCube {
    public:
        explicit OpenGLTextureCube(const std::vector<std::string> &paths);
        explicit OpenGLTextureCube(const TextureSpecification &specification);
        virtual ~OpenGLTextureCube();

        virtual const TextureSpecification &GetSpecification() const override { return m_Specification; }

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }

        virtual uint32_t GetRendererID() const override { return m_RendererID; }

        virtual const std::string &GetPath() const override { return m_Path; }

        virtual void SetData(void *data, uint32_t size) override;
        virtual void SetDataRegion(
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data,
                uint32_t size) override;

        virtual void SetFaceData(uint32_t faceIndex, uint32_t mipLevel, const void *data, uint32_t size) override;
        virtual uint32_t GetMipLevelCount() const override { return m_MipLevelCount; }

        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool IsLoaded() const override { return m_IsLoaded; }

        virtual bool operator==(const Texture &other) const override
        {
            return m_RendererID == other.GetRendererID();
        };

    private:
        void CreateEmptyStorage(const TextureSpecification &specification);

        uint32_t m_RendererID = 0;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_MipLevelCount = 1;
        uint32_t m_BytesPerPixel = 4;
        GLenum m_InternalFormat = GL_RGB8;
        GLenum m_DataFormat = GL_RGB;
        GLenum m_DataType = GL_UNSIGNED_BYTE;
        TextureSpecification m_Specification;
        std::string m_Path;
        bool m_IsLoaded = false;
    };
} // namespace Himii
