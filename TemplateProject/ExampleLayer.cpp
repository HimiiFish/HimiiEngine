#include "Engine.h"
#include "imgui.h"
#include "ExampleLayer.h"

// 顶点着色器源码
const char *vertexShaderSource = R"(#version 410 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
}
)";

// 片段着色器源码
const char *fragmentShaderSource = R"(#version 410 core
in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)";

ExampleLayer::ExampleLayer() : Layer("ExampleLayer")
{
    // Renderer
    m_VertexArray.reset(Himii::VertexArray::Create());
   
    float vertices[] = {
            // 位置          // 颜色
            -1.0f, -0.5f, 1.0f, 0.5f, 0.0f, // 左下角红色
            0.0f,  -0.5f, 0.0f, 1.0f, 0.5f, // 右下角绿色
            -0.5f, 0.5f,  0.5f, 0.0f, 1.0f  // 顶部蓝色
    };
    std::shared_ptr<Himii::VertexBuffer> m_VertexBuffer;
    m_VertexBuffer.reset(Himii::VertexBuffer::Create(vertices, sizeof(vertices)));

    Himii::BufferLayout layout = {{Himii::ShaderDataType::Float2, "aPos"}, {Himii::ShaderDataType::Float3, "aColor"}};
    m_VertexBuffer->SetLayout(layout);
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    unsigned int indices[] = {0, 1, 2}; // 三角形的索引
    std::shared_ptr<Himii::IndexBuffer> m_IndexBuffer;
    m_IndexBuffer.reset(Himii::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);

    m_SquareVA.reset(Himii::VertexArray::Create());

    float squareVertices[] = {
            // 位置          // 颜色
            0.0f, -0.5f, 1.0f,  0.75f, 0.5f,  // 左下角红色
            1.0f, -0.5f, 0.75f, 0.5f,  0.25f, // 右下角绿色
            1.0f, 0.5f,  0.5f,  0.25f, 1.0f,  // 右上角蓝色
            0.0f, 0.5f,  0.25f, 0.75f, 0.5f   // 左上角白色
    };
    // 创建顶点缓冲区
    std::shared_ptr<Himii::VertexBuffer> squareVB;
    squareVB.reset(Himii::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));

    Himii::BufferLayout squareLayout = {{Himii::ShaderDataType::Float2, "aPos"},
                                        {Himii::ShaderDataType::Float3, "aColor"}};
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
    m_Shader.reset(new Himii::Shader(vertexShaderSource, fragmentShaderSource));
}

void ExampleLayer::OnAttach()
{
}

void ExampleLayer::OnDetach()
{
}

void ExampleLayer::OnUpdate()
{
    if (Himii::Input::IsKeyPressed(Himii::Key::Space))
    {
        HIMII_INFO("Space key is pressed!");
    }

    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    Himii::Renderer::BeginScene();

    m_Shader->Bind();
    Himii::Renderer::Submit(m_VertexArray);

    Himii::Renderer::Submit(m_SquareVA);

    Himii::Renderer::EndScene();
}

void ExampleLayer::OnImGuiRender()
{
    ImGui::Text("测试窗口");
    if (ImGui::Button("点击"))
    {
        HIMII_INFO("Button clicked!");
    }
}

void ExampleLayer::OnEvent(Himii::Event &event)
{
    // 事件处理代码
    if (event.GetEventType() == Himii::EventType::KeyPressed)
    {
        Himii::KeyPressedEvent &keyEvent = static_cast<Himii::KeyPressedEvent &>(event);
        if (keyEvent.GetKeyCode() == Himii::Key::Tab)
        {
            HIMII_INFO_F("Tab key pressed");
        }
        HIMII_INFO_F("Key Pressed: {0}", (char)keyEvent.GetKeyCode());
    }
}
