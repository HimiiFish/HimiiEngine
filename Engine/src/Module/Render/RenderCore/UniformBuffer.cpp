#include "Hepch.h"
#include "Module/Render/RenderCore/UniformBuffer.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
    {
        return RHI::CreateUniformBuffer(size, binding);
    }
}
