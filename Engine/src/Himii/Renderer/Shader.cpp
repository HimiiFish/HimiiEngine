#include "Hepch.h"
#include "Shader.h"
#include "Himii/Core/Log.h"

#include "glad/glad.h"

namespace Himii
{
    Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource)
    {
        // 创建着色器对象
        uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
        uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        // 设置着色器源码
        const char *vertexSourceCStr = vertexSource.c_str();
        const char *fragmentSourceCStr = fragmentSource.c_str();
        glShaderSource(vertexShader, 1, &vertexSourceCStr, nullptr);
        glShaderSource(fragmentShader, 1, &fragmentSourceCStr, nullptr);
        // 编译着色器
        glCompileShader(vertexShader);
        glCompileShader(fragmentShader);
        // 检查编译错误
        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
            HIMII_CORE_ERROR("Vertex Shader Compilation Failed: {0}", infoLog);
        }
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
            HIMII_CORE_ERROR("Fragment Shader Compilation Failed: {0}", infoLog);
        }
        // 创建程序并链接着色器
        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);
        // 删除着色器对象，因为它们已经被链接到程序中
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_RendererID);
    }

    void Shader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }
}