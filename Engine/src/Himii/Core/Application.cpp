#include "Application.h"
#include "Hepch.h"
#include "Input.h"
#include "LayerStack.h"
#include "Log.h"
#include "glad/glad.h"

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

namespace Himii
{
    Application *Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        // Renderer
        m_VertexArray.reset(VertexArray::Create());

        float vertices[] = {
                // 位置          // 颜色
                -1.0f, -0.5f, 1.0f, 0.5f, 0.0f, // 左下角红色
                0.0f,  -0.5f, 0.0f, 1.0f, 0.5f, // 右下角绿色
                -0.5f,  0.5f,  0.5f, 0.0f, 1.0f  // 顶部蓝色
        };
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        m_VertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        BufferLayout layout = {{ShaderDataType::Float2, "aPos"}, {ShaderDataType::Float3, "aColor"}};
        m_VertexBuffer->SetLayout(layout);
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        unsigned int indices[] = {0, 1, 2}; // 三角形的索引
        std::shared_ptr<IndexBuffer> m_IndexBuffer;
        m_IndexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t)));
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        m_SquareVA.reset(VertexArray::Create());

        float squareVertices[] = {
                // 位置          // 颜色
                0.0f, -0.5f, 1.0f, 0.75f, 0.5f, // 左下角红色
                1.0f,  -0.5f, 0.75f, 0.5f, 0.25f, // 右下角绿色
                1.0f,  0.5f,  0.5f, 0.25f, 1.0f, // 右上角蓝色
                0.0f, 0.5f,  0.25f, 0.75f, 0.5f  // 左上角白色
        };
        // 创建顶点缓冲区
        std::shared_ptr<VertexBuffer> squareVB;
        squareVB.reset(VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
        
        BufferLayout squareLayout = {{ShaderDataType::Float2, "aPos"}, {ShaderDataType::Float3, "aColor"}};
        squareVB->SetLayout(squareLayout);
        m_SquareVA->AddVertexBuffer(squareVB);
        // 设置索引缓冲区
        uint32_t squareIndices[] = {0, 1, 2, 2, 3, 0}; // 两个三角形组成的正方形
        std::shared_ptr<IndexBuffer> squareIB;
        squareIB.reset(IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
        m_SquareVA->SetIndexBuffer(squareIB);

        // glBindBuffer(GL_ARRAY_BUFFER, 0);
        // glBindVertexArray(0);
        //  创建着色器程序
        m_Shader.reset(new Shader(vertexShaderSource, fragmentShaderSource));
    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClosed));

        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            if (e.Handled)
                break;
            (*--it)->OnEvent(e);
        }
    }

    bool Application::OnWindowClosed(WindowCloseEvent &e)
    {
        m_Running = false;
        return true;
    }

    void Application::Run()
    {
        while (m_Running)
        {
            //Renderer Update
            glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            m_Shader->Bind();

            m_VertexArray->Bind();
            glDrawElements(GL_TRIANGLES, m_VertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);

            m_SquareVA->Bind();
            glDrawElements(GL_TRIANGLES, m_SquareVA->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);

            //Layer Update
            for (Layer *layer: m_LayerStack)
            {
                layer->OnUpdate();
            }
            m_ImGuiLayer->Begin();
            for (Layer *layer: m_LayerStack)
            {
                layer->OnImGuiRender();
            }
            m_ImGuiLayer->End();

            //Window Update
            m_Window->Update();
        }
    }

} // namespace Himii
