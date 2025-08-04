#include "Example2D.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Example2D::Example2D() : Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f,0.38f,0.64f,1.0f})
{

}

void Example2D::OnAttach()
{
    m_SquareVA = Himii::VertexArray::Create();
    float squareVertices[] = {
            0.0f, -0.5f, 0.0f,  
            1.0f, -0.5f, 0.0f, 
            1.0f, 0.5f,  0.0f,  
            0.0f, 0.5f,  0.0f 
    };
    // 创建顶点缓冲区
    Himii::Ref<Himii::VertexBuffer> squareVB;
    squareVB.reset(Himii::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

    squareVB->SetLayout({{Himii::ShaderDataType::Float3, "a_Position"}});
    m_SquareVA->AddVertexBuffer(squareVB);
    // 设置索引缓冲区
    uint32_t squareIndices[] = {0, 1, 2, 2, 3, 0}; // 两个三角形组成的正方形
    std::shared_ptr<Himii::IndexBuffer> squareIB;
    squareIB.reset(Himii::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
    m_SquareVA->SetIndexBuffer(squareIB);

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
    //  创建着色器程序
    m_Shader = Himii::Shader::Create("assets/shaders/FlatColor.glsl");
}
void Example2D::OnDetach()
{

}

void Example2D::OnUpdate(Himii::Timestep ts)
{
    m_CameraController.OnUpdate(ts);

    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    m_Shader->Bind();
    m_Shader->SetFloat4("u_Color", m_SquareColor);
    Himii::Renderer::BeginScene(m_CameraController.GetCamera());

    Himii::Renderer::Submit(m_Shader, m_SquareVA);

    Himii::Renderer::EndScene();
}
void Example2D::OnImGuiRender()
{
    ImGui::Begin("设置");
    ImGui::Text("颜色设置");
    ImGui::ColorEdit4("矩形颜色", glm::value_ptr(m_SquareColor));
    ImGui::End();
}
void Example2D::OnEvent(Himii::Event& event)
{
    m_CameraController.OnEvent(event);
}
