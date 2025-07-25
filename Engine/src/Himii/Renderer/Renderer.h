#pragma once
#include "Himii/Renderer/RenderCommand.h"
#include "Himii/Renderer/OrthographicCamera.h"
#include "Himii/Renderer/Shader.h"

namespace Himii
{
    class Renderer
    {
    public:
        static void BeginScene(OrthographicCamera& camera);
        static void EndScene();

        static void Submit(const Ref<Shader> &shader, const Ref<VertexArray> &vertexArray);

        inline static RendererAPI::API GetAPI()
        {
            return RendererAPI::GetAPI();
        }

    private:
        struct SceneData 
        {
            glm::mat4 ViewProjectionMatrix;
        };

        static Scope<SceneData> m_SceneData;
    };
}