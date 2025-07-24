#include "Application.h"
#include "Hepch.h"
#include "Input.h"
#include "glad/glad.h"
#include "Log.h"
#include "LayerStack.h"

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

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
    {
        switch (type)
        {
            case Himii::ShaderDataType::Float:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float2:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float3:
                return GL_FLOAT;
            case Himii::ShaderDataType::Float4:
                return GL_FLOAT;
            case Himii::ShaderDataType::Mat3:
                return GL_FLOAT;
            case Himii::ShaderDataType::Mat4:
                return GL_FLOAT;
            case Himii::ShaderDataType::Int:
                return GL_INT;
            case Himii::ShaderDataType::Int2:
                return GL_INT;
            case Himii::ShaderDataType::Int3:
                return GL_INT;
            case Himii::ShaderDataType::Int4:
                return GL_INT;
            case Himii::ShaderDataType::Bool:
                return GL_INT;
        }
        HIMII_CORE_ASSERT(false, "Unknonw ShaderDataType");
        return 0;
    }

    Application::Application()
    {
        s_Instance = this;
        m_Window = Window::Create();
        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        //test opengl
        glGenVertexArrays(1, &m_VertexArray);
        glBindVertexArray(m_VertexArray);

        float vertices[]=
        {
            // 位置          // 颜色
            -0.5f, -0.5f, 1.0f, 0.5f, 0.0f, // 左下角红色
             0.5f, -0.5f, 0.0f, 1.0f, 0.5f, // 右下角绿色
             0.0f,  0.5f, 0.5f, 0.0f, 1.0f  // 顶部蓝色
        };
        m_VertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        BufferLayout layout = {
            {ShaderDataType::Float2, "aPos"},
            {ShaderDataType::Float3, "aColor"}
        };
        m_VertexBuffer->SetLayout(layout);
        uint32_t index = 0;
        for (const auto& element : layout)
        {
            glEnableVertexAttribArray(index); // 位置属性
            glVertexAttribPointer(index, 
                element.GetCompomentCount(), 
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized? GL_TRUE:GL_FALSE, 
                layout.GetStride(), 
                (const void *)element.Offset);
            index++;
        }
        unsigned int indices[] = {0, 1, 2}; // 三角形的索引
        m_IndexBuffer.reset(IndexBuffer::Create(indices, 3));

        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        //glBindVertexArray(0);
        // 创建着色器程序
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
            glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            glBindVertexArray(m_VertexArray);
            m_Shader->Bind();
            glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, 0);

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

            m_Window->Update();
        }
    }

} // namespace Core
