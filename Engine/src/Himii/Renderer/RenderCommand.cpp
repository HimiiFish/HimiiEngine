#include "Hepch.h"
#include "Himii/Renderer/RenderCommand.h"

namespace Himii
{
    Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}