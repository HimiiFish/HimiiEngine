#include "Hepch.h"
#include "Renderer.h"

namespace Himii
{
    void Renderer::BeginScene()
    {
    }
    void Renderer::EndScene()
    {
    }
    void Renderer::Submit(const Ref<VertexArray> &vertexArray)
    {
        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }
} // namespace Himii
