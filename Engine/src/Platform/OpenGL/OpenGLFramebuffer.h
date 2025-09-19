#pragma once
#include "Himii/Renderer/Framebuffer.h"
#include "glad/glad.h"

namespace Himii {
    class OpenGLFramebuffer : public Framebuffer {
    public:
        OpenGLFramebuffer(const FramebufferSpecification& spec);
        ~OpenGLFramebuffer();

        void Bind() override;
        void Unbind() override;

        void Resize(uint32_t width, uint32_t height) override;
        uint32_t GetColorAttachmentRendererID() const override { return m_ColorAttachment; }
        const FramebufferSpecification& GetSpecification() const override { return m_Spec; }

    private:
        void Invalidate();

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0;
        uint32_t m_DepthStencilAttachment = 0; // RBO
        FramebufferSpecification m_Spec;
    };
}
