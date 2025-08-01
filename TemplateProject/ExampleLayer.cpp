#include "ExampleLayer.h"
#include "Engine.h"
#include "imgui.h"


#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

// 顶点着色器源码
const char *vertexShaderSource = R"(#version 410 core
layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection; // 视图投影矩阵
uniform mat4 u_Transform; // 变换矩阵

out vec3 v_Position;

void main()
{
    v_Position=a_Position;
    gl_Position = u_ViewProjection*u_Transform* vec4(a_Position, 1.0);
}
)";

// 片段着色器源码
const char *fragmentShaderSource = R"(#version 410 core

layout(location=0) out vec4 FragColor;
in vec3 v_Position;

uniform vec4 u_Color; // 颜色

void main()
{
    FragColor = u_Color;
}
)";

ExampleLayer::ExampleLayer() : Layer("ExampleLayer"), m_CameraController(1280.0f/720.0f) // 设置正交相机的视口
{
    // Renderer
    m_VertexArray.reset(Himii::VertexArray::Create());

    m_SquareColor1 = glm::vec4(0.29f, 0.41f, 0.6f, 1.0f);
    m_SquareColor2 = glm::vec4(0.6f, 0.28f, 0.29f, 1.0f);
    float vertices[] = {
            // 位置          // 颜色
            -1.0f, -0.5f, 1.0f, 0.5f, 0.0f, // 左下角红色
            0.0f,  -0.5f, 0.0f, 1.0f, 0.5f, // 右下角绿色
            -0.5f, 0.5f,  0.5f, 0.0f, 1.0f  // 顶部蓝色
    };
    std::shared_ptr<Himii::VertexBuffer> m_VertexBuffer;
    m_VertexBuffer.reset(Himii::VertexBuffer::Create(vertices, sizeof(vertices)));

    Himii::BufferLayout layout = {{Himii::ShaderDataType::Float3, "a_Position"},
                                  {Himii::ShaderDataType::Float2, "a_TexCoord"}};
    m_VertexBuffer->SetLayout(layout);
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    unsigned int indices[] = {0, 1, 2}; // 三角形的索引
    std::shared_ptr<Himii::IndexBuffer> m_IndexBuffer;
    m_IndexBuffer.reset(Himii::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);

    m_SquareVA.reset(Himii::VertexArray::Create());

    float squareVertices[] = {
            // 位置          // 颜色
            0.0f, -0.5f, 0.0f, 0.0f, 0.0f, // 左下角红色
            1.0f, -0.5f, 0.0f, 1.0f, 0.0f, // 右下角绿色
            1.0f, 0.5f,  0.0f, 1.0f, 1.0f, // 右上角蓝色
            0.0f, 0.5f,  0.0f, 0.0f, 1.0f  // 左上角白色
    };
    // 创建顶点缓冲区
    std::shared_ptr<Himii::VertexBuffer> squareVB;
    squareVB.reset(Himii::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

    Himii::BufferLayout squareLayout = {
            {Himii::ShaderDataType::Float3, "a_Position"},
            {Himii::ShaderDataType::Float2, "a_TexCoord"}
    };
    squareVB->SetLayout(squareLayout);
    m_SquareVA->AddVertexBuffer(squareVB);
    // 设置索引缓冲区
    uint32_t squareIndices[] = {0, 1, 2, 2, 3, 0}; // 两个三角形组成的正方形
    std::shared_ptr<Himii::IndexBuffer> squareIB;
    squareIB.reset(Himii::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
    m_SquareVA->SetIndexBuffer(squareIB);

    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);
    //  创建着色器程序
    m_Shader = Himii::Shader::Create("squareShader", vertexShaderSource, fragmentShaderSource);
    auto m_TextureShader = m_ShaderLibrary.Load("assets/shaders/Texture.glsl");

    m_Texture=Himii::Texture2D::Create("assets/textures/blocks.png");

    m_TextureShader->Bind();
    m_TextureShader->SetInt("u_Texture", 0);
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate(Himii::Timestep ts)
{
    m_CameraController.OnUpdate(ts);

    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    Himii::Renderer::BeginScene(m_CameraController.GetCamera());

    static glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));

    for (int x = 0; x < 10; ++x)
    {
        for (int y = 0; y < 10; y++)
        {
            glm::vec3 pos(x * 0.11f, y * 0.11f, 0.0f);
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
            if ((x + y) % 2 == 0)
            {
                m_Shader->SetFloat4("u_Color", m_SquareColor1);
            }
            else
            {
                m_Shader->SetFloat4("u_Color", m_SquareColor2);
            }

            Himii::Renderer::Submit(m_Shader, m_SquareVA, transform);
        }
    }
    m_Texture->Bind();

    Himii::Renderer::Submit(m_Shader, m_SquareVA, glm::scale(glm::mat4(1.0f), glm::vec3(1.5f)));

    Himii::Renderer::Submit(m_ShaderLibrary.Get("Texture"), m_SquareVA,
                            glm::translate(glm::mat4(1.0f), glm::vec3(-0.25f, 0.0f, 0.0f)) *
                                    glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.6f, 1.0f)));
    // Himii::Renderer::Submit(m_Shader,m_VertexArray);

    Himii::Renderer::EndScene();
}

void ExampleLayer::OnImGuiRender()
{
    ImGui::Begin("Settings");
    ImGui::ColorEdit3("Square Color1", glm::value_ptr(m_SquareColor1));
    ImGui::ColorEdit3("Square Color2", glm::value_ptr(m_SquareColor2));
    ImGui::Text("交换颜色");
    if (ImGui::Button("点击"))
    {
        glm::vec4 bridge;
        bridge = m_SquareColor1;
        m_SquareColor1 = m_SquareColor2;
        m_SquareColor2 = bridge;
    }
    ImGui::End();
}

void ExampleLayer::OnEvent(Himii::Event &event)
{
    m_CameraController.OnEvent(event);
}
