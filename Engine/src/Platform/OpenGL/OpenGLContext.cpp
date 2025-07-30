#include"Hepch.h"
#include "Himii/Core/Log.h"
#include "OpenGLContext.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"


namespace Himii
{
    OpenGLContext::OpenGLContext(GLFWwindow *windowHandle) : m_WindowHandle(windowHandle)
    {
        HIMII_CORE_ASSERT(windowHandle, "Window handle is null!");
    }

    void OpenGLContext::Init()
    {
        glfwMakeContextCurrent(m_WindowHandle);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        HIMII_CORE_ASSERT(status, "Failed to initialize GLAD");

        auto a = glGetString(GL_VERSION);
        std::cout << "OpenGL Version: " << a << "\n" << "Renderer Version:" << glGetString(GL_RENDERER) << std::endl;
        HIMII_CORE_INFO("OpenGL Info:");
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_WindowHandle);
    }
}