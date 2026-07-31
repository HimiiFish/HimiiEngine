#include "Hepch.h"
#include "Module/Render/RenderCore/Buffer.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        return RHI::CreateVertexBuffer(size);
    }

    Ref<VertexBuffer> VertexBuffer::Create(float *vertices, uint32_t size)
    {
        return RHI::CreateVertexBuffer(vertices, size);
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t *indices, uint32_t count)
    {
        return RHI::CreateIndexBuffer(indices, count);
    }
}
