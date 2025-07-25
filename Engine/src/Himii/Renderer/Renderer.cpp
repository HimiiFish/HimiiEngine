#include "Hepch.h"
#include "Renderer.h"

namespace Himii
{
    Scope<Renderer::SceneData> Renderer::m_SceneData = CreateScope<Renderer::SceneData>();

    void Renderer::BeginScene(OrthographicCamera& camera)
    {
        m_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    }
    void Renderer::EndScene()
    {
    }
    void Renderer::Submit(const Ref<Shader> &shader, const Ref<VertexArray> &vertexArray)
    {
        shader->Bind();
        shader->UploadUniformMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
        vertexArray->Bind();
        RenderCommand::DrawIndexed(vertexArray);
    }
} // namespace Himii
