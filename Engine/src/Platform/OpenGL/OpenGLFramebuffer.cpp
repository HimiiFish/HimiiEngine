#include "Hepch.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Himii/Core/Log.h"
#include "glad/glad.h"

namespace Himii {

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification &spec)
            : m_Spec(spec)
    {
        Invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        if (m_DepthStencilAttachment) glDeleteRenderbuffers(1, &m_DepthStencilAttachment);
        if (m_ColorAttachment) glDeleteTextures(1, &m_ColorAttachment);
        if (m_RendererID) glDeleteFramebuffers(1, &m_RendererID);
    }

    void OpenGLFramebuffer::Invalidate()
    {
        if (m_DepthStencilAttachment) glDeleteRenderbuffers(1, &m_DepthStencilAttachment);
        if (m_ColorAttachment) glDeleteTextures(1, &m_ColorAttachment);
        if (m_RendererID) glDeleteFramebuffers(1, &m_RendererID);

        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        // Color texture
        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Spec.Width, m_Spec.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        // Depth-stencil renderbuffer
        glGenRenderbuffers(1, &m_DepthStencilAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilAttachment);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Spec.Width, m_Spec.Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthStencilAttachment);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            HIMII_CORE_ERROR("Framebuffer incomplete!");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, (GLsizei)m_Spec.Width, (GLsizei)m_Spec.Height);
    }

    void OpenGLFramebuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) return;
        m_Spec.Width = width; m_Spec.Height = height;
        Invalidate();
    }
}
