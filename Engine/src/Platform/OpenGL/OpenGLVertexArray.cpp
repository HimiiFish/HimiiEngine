#include "Hepch.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "glad/glad.h"

namespace Himii
{
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case Himii::ShaderDataType::Float:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float2:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float3:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float4:
                return GL_FLOAT;
            case Himii::ShaderDataType::Mat3:
                return GL_FLOAT;
            case Himii::ShaderDataType::Mat4:
                return GL_FLOAT;
            case Himii::ShaderDataType::Int:
                return GL_INT;
            case Himii::ShaderDataType::Int2:
                return GL_INT;
            case Himii::ShaderDataType::Int3:
                return GL_INT;
            case Himii::ShaderDataType::Int4:
                return GL_INT;
            case Himii::ShaderDataType::Bool:
                return GL_INT;
        }
        HIMII_CORE_ASSERT(false, "Unknonw ShaderDataType");
        return 0;
    }

    OpenGLVertexArray::OpenGLVertexArray()
    {
        glCreateVertexArrays(1, &m_RendererID);
    }
    OpenGLVertexArray::~OpenGLVertexArray()
    {
        glDeleteVertexArrays(1, &m_RendererID);
    }
    void OpenGLVertexArray::Bind() const
    {
        glBindVertexArray(m_RendererID);
    }
    void OpenGLVertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }
    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer)
    {
        glBindVertexArray(m_RendererID);
        vertexBuffer->Bind();

        HIMII_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size() > 0,
                          "Vertex Buffer has no layout! Please set the layout before binding it to the VertexArray!");

        const auto &layout = vertexBuffer->GetLayout();
        uint32_t index = 0;
        for (const auto &element: layout)
        {
            glEnableVertexAttribArray(index); // Enable the vertex attribute array
            glVertexAttribPointer(index, element.GetCompomentCount(), ShaderDataTypeToOpenGLBaseType(element.Type),
                                  element.Normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
                                  (const void *)element.Offset);
            index++;
        }
        m_VertexBuffers.push_back(vertexBuffer);
    }
    void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer)
    {
        glBindVertexArray(m_RendererID);
        indexBuffer->Bind();

        m_IndexBuffers = indexBuffer;
    }
} // namespace Himii
