#pragma once
#include "Himii/Renderer/Texture.h"

#include "glad/glad.h"

namespace Himii
{
	class OpenGLTexture:public Texture2D 
    {
        // ͨ�� Texture2D �̳�
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

    // Atlas UV helpers
    virtual std::array<glm::vec2,4> GetUVFromGrid(int col, int row, int cols, int rows, float paddingNorm = 0.0f) const override;
    virtual std::array<glm::vec2,4> GetUVFromPixels(const glm::vec2& pixelMin,
                            const glm::vec2& pixelMax,
                            const glm::vec2& paddingPx = {0.0f, 0.0f}) const override;

    private:
        std::string m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_RendererID;
        GLenum m_InternalFormat, m_DataFormat;
    };
}