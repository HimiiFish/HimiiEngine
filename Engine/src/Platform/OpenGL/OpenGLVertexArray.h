#pragma once
#include "Himii/Renderer/VertexArray.h"

namespace Himii
{
    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();
        virtual ~OpenGLVertexArray();

        virtual void Bind() const override;
        virtual void Unbind() const override;


        virtual void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) override;
        virtual void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) override;

        virtual const std::vector<Ref<VertexBuffer>> &GetVertexBuffers() const
        {
            return m_VertexBuffers;
        };
        virtual const Ref<IndexBuffer> &GetIndexBuffer() const
        {
            return m_IndexBuffers;
        };

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_VertexBufferIndex = 0;
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffers;
    };
}