#include "Hepch.h"
#include "Module/Render/RenderCore/VertexArray.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<VertexArray> VertexArray::Create()
    {
        return RHI::CreateVertexArray();
    }
}
