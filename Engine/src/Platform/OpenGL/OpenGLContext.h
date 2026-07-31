#pragma once
#include "Module/Render/GraphicsContext.h"

struct GLFWwindow;

namespace Himii
{
    class OpenGLContext : public GraphicsContext
    {
    public:
        OpenGLContext(GLFWwindow *windowHandle);
        virtual ~OpenGLContext() = default;
        virtual void Init() override;
        virtual void SwapBuffers() override;
    private:
        GLFWwindow* m_WindowHandle;
    };
}
