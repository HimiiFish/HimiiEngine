#include "Hepch.h"
#include "Himii/Core/Log.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "fstream"
#include "glm/gtc/type_ptr.hpp"

namespace Himii
{
    static GLenum ShaderTypeFromString(const std::string &type)
    {
        if (type == "vertex")
            return GL_VERTEX_SHADER;
        if (type == "fragment" || type == "pixel")
            return GL_FRAGMENT_SHADER;

        HIMII_CORE_ASSERT(false, "Unknown shader type!");
        return 0;
    }

    OpenGLShader::OpenGLShader(const std::string &filepath)
    {
        std::string shaderSource = ReadFile(filepath);
        auto shaderSources = PreProcess(shaderSource);
        Compile(shaderSources);

        // Extract Name from filepath
        auto lastSlash = filepath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filepath.rfind('.');
        auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filepath.substr(lastSlash, count);
    }
    OpenGLShader::OpenGLShader(const std::string &name, const std::string &vertexSource,
                               const std::string &fragmentSource) : m_Name(name)
    {
        std::unordered_map<GLenum, std::string> sources;
        sources[GL_VERTEX_SHADER] = vertexSource;
        sources[GL_FRAGMENT_SHADER] = fragmentSource;
        Compile(sources);
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteProgram(m_RendererID);
    }

    std::string OpenGLShader::ReadFile(const std::string &filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in|std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&result[0], result.size());
            in.close();
        }
        else
        {
            HIMII_CORE_ERROR("Could no open file '{0}'", filepath);
        }
        return result;
    }

    std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string &source)
    {
        std::unordered_map<GLenum, std::string> shaderSources;

        const char *typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0); // Start of shader type declaration line
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos); // End of shader type declaration line
            HIMII_CORE_ASSERT(eol != std::string::npos, "Syntax error");
            size_t begin = pos + typeTokenLength + 1; // Start of shader type name (after "#type " keyword)
            std::string type = source.substr(begin, eol - begin);
            HIMII_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

            size_t nextLinePos =
                    source.find_first_not_of("\r\n", eol); // Start of shader code after shader type declaration line
            HIMII_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
            pos = source.find(typeToken, nextLinePos); // Start of next shader type declaration line

            shaderSources[ShaderTypeFromString(type)] =
                    source.substr(nextLinePos, pos - (nextLinePos == std::string::npos?source.size()-1:nextLinePos));
        }

        return shaderSources;
    }

    void OpenGLShader::Compile(std::unordered_map<GLenum, std::string> &shaderSources)
    {
        GLint program = glCreateProgram();
        HIMII_CORE_ASSERT(shaderSources.size() <= 2, "We only support 2 shaders for now");
        std::array<GLenum, 2> glShaderIDs;
        int glShaderIDIndex = 0;

        for (auto &kv: shaderSources)
        {
            GLenum type = kv.first;
            std::string &source = kv.second;

            GLuint shader = glCreateShader(type);
            // ÉèÖÃ×ÅÉ«Æ÷Ô´Âë
            const GLchar *SourceCStr = source.c_str();
            glShaderSource(shader, 1, &SourceCStr, 0);
            // ±àÒë×ÅÉ«Æ÷
            glCompileShader(shader);
            // ¼ì²é±àÒë´íÎó
            int success=0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success==GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

                glDeleteShader(shader);

                HIMII_CORE_ERROR("{0}", infoLog.data());
                HIMII_CORE_ASSERT(false, "Shader compilation failure!");
                break;
            }
            glAttachShader(program, shader);
            glShaderIDs[glShaderIDIndex++]=shader;
        }

        m_RendererID = program;
        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

            glDeleteProgram(program);
            for (auto id: glShaderIDs)
            {
                glDeleteShader(id);
            }
            HIMII_CORE_ERROR("{0}", infoLog.data());
            HIMII_CORE_ASSERT(false, "Shader link failure!");
            return;
        }

        for (auto id: glShaderIDs)
        {
            glDetachShader(program, id);
        }
    }

    void OpenGLShader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::Unbind() const
    {
        glUseProgram(0);
    }

    void OpenGLShader::SetInt(const std::string &name, int value)
    {
        UploadUniformInt(name, value);
    }

    void OpenGLShader::SetIntArray(const std::string &name, int *values, uint32_t count)
    {
        UploadUniformIntArray(name, values, count);
    }

    void OpenGLShader::SetFloat(const std::string &name, float value)
    {
        UploadUniformFloat(name, value);
    }

    void OpenGLShader::SetFloat2(const std::string &name, const glm::vec2 &value)
    {
        UploadUniformFloat2(name, value);
    }

    void OpenGLShader::SetFloat3(const std::string &name, const glm::vec3 &value)
    {
        UploadUniformFloat3(name, value);
    }

    void OpenGLShader::SetFloat4(const std::string &name, const glm::vec4 &value)
    {
        UploadUniformFloat4(name, value);
    }

    void OpenGLShader::SetMat4(const std::string &name, const glm::mat4 &value)
    {
        UploadUniformMat4(name, value);
    }

    void OpenGLShader::UploadUniformInt(const std::string &name, int value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, value);
    }

    void OpenGLShader::UploadUniformIntArray(const std::string &name, int *values, uint32_t count)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1iv(location, count, values);
    }

    void OpenGLShader::UploadUniformFloat(const std::string &name, float value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1f(location, value);
    }

    void OpenGLShader::UploadUniformFloat2(const std::string &name, const glm::vec2 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform2f(location, value.x, value.y);
    }

    void OpenGLShader::UploadUniformFloat3(const std::string &name, const glm::vec3 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform3f(location, value.x, value.y, value.z);
    }

    void OpenGLShader::UploadUniformFloat4(const std::string &name, const glm::vec4 &value)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void OpenGLShader::UploadUniformMat3(const std::string &name, const glm::mat3 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void OpenGLShader::UploadUniformMat4(const std::string &name, const glm::mat4 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }


} // namespace Himii
