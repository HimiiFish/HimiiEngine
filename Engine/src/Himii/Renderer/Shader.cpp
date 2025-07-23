#include "Hepch.h"
#include "Shader.h"

#include "glad/glad.h"

namespace Himii
{
    Shader::Shader(const std::string &vertexSource, const std::string &fragmentSource)
    {
        //m_RendererID = CreateShaderProgram(vertexSource, fragmentSource);
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