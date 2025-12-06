#include "Hepch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"

#include "glad/glad.h"
namespace Himii
{
    void OpenGLMessageCallback(unsigned source, unsigned type, unsigned id, unsigned severity, int length,
                               const char *message, const void *userParam)
    {
        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:
                HIMII_CORE_ERROR(message);
                return;
            case GL_DEBUG_SEVERITY_MEDIUM:
                HIMII_CORE_WARNING(message);
                return;
            case GL_DEBUG_SEVERITY_LOW:
                HIMII_CORE_TRACE(message);
                return;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                HIMII_CORE_INFO(message);
                return;
        }

        HIMII_CORE_ASSERT(false, "Unknown severity level!");
    }
    void OpenGLRendererAPI::Init()
    {
        HIMII_PROFILE_FUNCTION();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LINE_SMOOTH);
    }

    void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void OpenGLRendererAPI::SetClearColor(const glm::vec4 &color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRendererAPI::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray> &vertexArray, uint32_t indexCount)
    {
        vertexArray->Bind();
        uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }
} // namespace Himii
