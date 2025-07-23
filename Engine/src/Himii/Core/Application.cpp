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
    // 编译着色器并创建程序
    GLuint CompileShader(GLenum type, const char *source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        // 检查错误
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            std::cerr << "Shader Compile Error:\n" << log << std::endl;
        }
        return shader;
    }

    GLuint CreateShaderProgram()
    {
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        // 检查链接错误
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(program, 512, nullptr, log);
            std::cerr << "Shader Link Error:\n" << log << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return program;
    }

    Application *Application::s_Instance = nullptr;

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
            -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // 左下角红色
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // 右下角绿色
             0.0f,  0.5f, 0.0f, 0.0f, 1.0f  // 顶部蓝色
        };
        m_VertexBuffer.reset(VertexBuffer::Create(vertices, sizeof(vertices)));

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0); // 位置属性
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1); // 颜色属性
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));

        unsigned int indices[] = {0, 1, 2}; // 三角形的索引
        m_IndexBuffer.reset(IndexBuffer::Create(indices, 3));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        // 创建着色器程序
        GLuint shaderProgram = CreateShaderProgram();
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
            glUseProgram(CreateShaderProgram());
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
