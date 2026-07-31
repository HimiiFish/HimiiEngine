#include "Module/Render/Texture.h"

namespace Himii
{
    class OpenGLTextureCube : public TextureCube {
    public:
        OpenGLTextureCube(const std::vector<std::string> &path);
        virtual ~OpenGLTextureCube();

        virtual const TextureSpecification &GetSpecification() const override { return m_Specification; }

        virtual uint32_t GetWidth() const override { return m_Width; }
        virtual uint32_t GetHeight() const override { return m_Height; }
        
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
        
        virtual const std::string &GetPath() const override { return m_Path; }
        
        virtual void SetData(void *data, uint32_t size) override {}
        virtual void SetDataRegion(
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, void *data,
                uint32_t size) override
        {
            (void)offsetX;
            (void)offsetY;
            (void)width;
            (void)height;
            (void)data;
            (void)size;
        }

        virtual void Bind(uint32_t slot = 0) const override;

        virtual bool IsLoaded() const override { return m_IsLoaded; }

        virtual bool operator==(const Texture &other) const override
        {
            return m_RendererID == other.GetRendererID();
        };

    private:
        uint32_t m_RendererID;
        uint32_t m_Width = 0, m_Height = 0;
        TextureSpecification m_Specification;
        std::string m_Path;
        bool m_IsLoaded = false;
    };
} // namespace Himii
