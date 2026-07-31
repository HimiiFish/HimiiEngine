#include "Hepch.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Module/Render/RHI/RHI.h"

namespace Himii
{
    Ref<Shader> Shader::Create(const std::string &filepath)
    {
        return RHI::CreateShader(filepath);
    }

    Ref<Shader> Shader::Create(
            const std::string &name, const std::string &vertexSource, const std::string &fragmentSource)
    {
        return RHI::CreateShader(name, vertexSource, fragmentSource);
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

    bool ShaderLibrary::Exists(const std::string &name)
    {
        return m_Shaders.find(name) != m_Shaders.end();
    }
}
