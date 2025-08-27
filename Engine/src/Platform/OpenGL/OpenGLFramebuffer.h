#pragma once
#include "Himii/Renderer/Framebuffer.h"
#include "Himii/Core/Log.h"
#include "glad/glad.h"

namespace Himii
{
    class OpenGLFramebuffer : public Framebuffer {
    public:
        OpenGLFramebuffer(const FramebufferSpecification &spec);
        ~OpenGLFramebuffer();

        void Invalidate();

        void Bind() override;
        void Unbind() override;

        virtual void Resize(uint32_t width, uint32_t height) override;
        virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

        virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;
        virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override
        {
            HIMII_CORE_ASSERT(index < m_ColorAttachments.size());
            return m_ColorAttachments[index];
        }

        virtual const FramebufferSpecification &GetSpecification() const override
        {
            return m_Specification;
        }

    private:
        uint32_t m_RendererID = 0;
        FramebufferSpecification m_Specification;

        std::vector<FramebufferTextureSpecification> m_ColorAttachmentSpecifications;
        FramebufferTextureSpecification m_DepthAttachmentSpecification = FramebufferFormat::None;

        std::vector<uint32_t> m_ColorAttachments;
        uint32_t m_DepthAttachment = 0;
    };
} // namespace Himii
