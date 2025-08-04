#include "Hepch.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Renderer/VertexArray.h"
#include "Himii/Renderer/Shader.h"
#include "Himii/Renderer/RenderCommand.h"

namespace Himii
{
    struct Renderer2DStorage 
    {
        Ref<VertexArray> QuadVertexArray;
        Ref<Shader> FlatColorShader;
    };

    static Renderer2DStorage* s_Data;

    void Renderer2D::Init()
    {
        s_Data = new Renderer2DStorage;

        s_Data->QuadVertexArray = VertexArray::Create();
        float squareVertices[] = {0.0f, -0.5f, 0.0f, 1.0f, -0.5f, 0.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f};
        // 创建顶点缓冲区
        Ref<VertexBuffer> squareVB;
        squareVB.reset(VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

        squareVB->SetLayout({{ShaderDataType::Float3, "a_Position"}});
        s_Data->QuadVertexArray->AddVertexBuffer(squareVB);
        // 设置索引缓冲区
        uint32_t squareIndices[] = {0, 1, 2, 2, 3, 0}; // 两个三角形组成的正方形
        Ref<IndexBuffer> squareIB;
        squareIB.reset(IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
        s_Data->QuadVertexArray->SetIndexBuffer(squareIB);

        // glBindBuffer(GL_ARRAY_BUFFER, 0);
        // glBindVertexArray(0);
        //  创建着色器程序
        s_Data->FlatColorShader = Shader::Create("assets/shaders/FlatColor.glsl");
    }
    void Renderer2D::Shutdown()
    {
        delete s_Data;
    }
    void Renderer2D::BeginScene(const OrthographicCamera &camera)
    {
        s_Data->FlatColorShader->Bind();
        s_Data->FlatColorShader->SetMat4("u_ViewProjection",camera.GetViewProjectionMatrix());
        s_Data->FlatColorShader->SetMat4("u_Transform",glm::mat4(1.0f));
    }
    void Renderer2D::EndScene()
    {
    }
    void Renderer2D::DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        DrawQuad({position.x, position.y, 0.0f}, size, color);
    }
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        s_Data->FlatColorShader->Bind();
        s_Data->FlatColorShader->SetFloat4("u_Color", color);
        s_Data->QuadVertexArray->Bind();
        RenderCommand::DrawIndexed(s_Data->QuadVertexArray);
    }
} // namespace Himii
