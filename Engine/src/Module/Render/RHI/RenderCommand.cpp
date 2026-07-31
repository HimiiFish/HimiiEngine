#include "Hepch.h"
#include "Module/Render/RHI/RenderCommand.h"

namespace Himii
{
    Scope<RHI> RenderCommand::s_RHI = RHI::Create();
}
