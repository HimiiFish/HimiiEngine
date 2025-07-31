#include "Hepch.h"
#include "Shader.h"

#include "Platform/OpenGL/OpenGLShader.h"
#include "Renderer.h"

namespace Himii
{
    Ref<Shader> Shader::Create(const std::string &filepath)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(filepath);
        }
    }

    Ref<Shader> Shader::Create(const std::string &name, const std::string &vertexSrc, const std::string &fragmentSrc)
    {
        switch (Renderer::GetAPI())
        {
            case RendererAPI::API::None:
                HIMII_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
                return nullptr;
            case RendererAPI::API::OpenGL:
                return CreateRef<OpenGLShader>(name, vertexSrc, fragmentSrc);
        }
    }

    void ShaderLibrary::Add(Ref<Shader> &shader)
    {
        auto name = shader->GetName();
        Add(name, shader);
    }

    void ShaderLibrary::Add(const std::string &name, const Ref<Shader> &shader)
    {
        m_Shaders[name] = shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string &filepath)
    {
        auto shader = Shader::Create(filepath);
        Add(shader);
        return shader;
    }
    Ref<Shader> ShaderLibrary::Load(const std::string &name, const std::string &filepath)
    {
        auto shader = Shader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string &name)
    {
        return m_Shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string& name)
    {
        return m_Shaders.find(name) != m_Shaders.end();
    }
} // namespace Himii
