#include "Application.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include "glad/glad.h"

// 顶点着色器源码
const char *vertexShaderSource = R"(#version 330 core
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
const char *fragmentShaderSource = R"(#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)";

namespace Core
{
    Application *Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;
    }

    Application::~Application()
    {
        s_Instance = nullptr;
    }

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

    void Application::Run()
    {
        while (m_Running)
        {
            // 初始化 SDL
            if (SDL_Init(SDL_INIT_VIDEO) < 0)
            {
                std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
                return;
            }

            // 设置 OpenGL 属性
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

            SDL_Window *window = SDL_CreateWindow("OpenGL Triangle", 800, 600, SDL_WINDOW_OPENGL);
            if (!window)
            {
                std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
                SDL_Quit();
                return;
            }

            SDL_GLContext glContext = SDL_GL_CreateContext(window);
            if (!glContext)
            {
                std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
                SDL_DestroyWindow(window);
                SDL_Quit();
                return;
            }

            if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
            {
                std::cerr << "Failed to initialize GLAD!" << std::endl;
                // SDL_GL_DeleteContext(glContext);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return;
            }

            // OpenGL 初始化
            // std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

            // 顶点数据：位置(x, y) + 颜色(r, g, b)
            float vertices[] = {
                    // positions     // colors
                    -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // left - red
                    0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, // right - green
                    0.0f,  0.5f,  0.0f, 0.0f, 1.0f  // top - blue
            };

            // 创建 VAO、VBO
            GLuint VAO, VBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            // 绑定 VAO
            glBindVertexArray(VAO);

            // 绑定 VBO 并传数据
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // 顶点位置属性
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);

            // 颜色属性
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            // 创建 shader 程序
            GLuint shaderProgram = CreateShaderProgram();

            // 设置清屏颜色
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

            SDL_Event event;
            while (m_Running)
            {
                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT)
                    {
                        m_Running = false;
                    }
                        
                }

                glClear(GL_COLOR_BUFFER_BIT);

                glUseProgram(shaderProgram);
                glBindVertexArray(VAO);
                glDrawArrays(GL_TRIANGLES, 0, 3);

                SDL_GL_SwapWindow(window);
            }

            // 清理
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteProgram(shaderProgram);

            SDL_DestroyWindow(window);
            SDL_Quit();
        }
    }

} // namespace Core
