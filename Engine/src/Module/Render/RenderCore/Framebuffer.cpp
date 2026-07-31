#include "Hepch.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification &specification)
    {
        return RHI::CreateFramebuffer(specification);
    }
}
