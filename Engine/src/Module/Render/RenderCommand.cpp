#include "Hepch.h"
#include "Module/Render/RenderCommand.h"

namespace Himii
{
    Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}