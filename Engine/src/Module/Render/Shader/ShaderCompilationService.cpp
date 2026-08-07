#include "Hepch.h"
#include "Module/Render/Shader/ShaderCompilationService.h"
#include "Module/Render/Shader/ShaderSourceUtility.h"
#include "Module/Render/RenderCore/Shader.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    Ref<Shader> ShaderCompilationService::GetOrCompileShader(const Ref<ShaderAsset> &shaderAsset)
    {
        if (!shaderAsset)
            return nullptr;

        if (shaderAsset->HasValidCompiledShader && shaderAsset->CompiledShader)
            return shaderAsset->CompiledShader;

        Ref<Shader> compiledShader;
        if (TryCompileShaderAsset(shaderAsset, compiledShader))
            return compiledShader;

        return shaderAsset->CompiledShader;
    }

    bool ShaderCompilationService::TryCompileShaderAsset(const Ref<ShaderAsset> &shaderAsset,
                                                         Ref<Shader> &outCompiledShader)
    {
        if (!shaderAsset || shaderAsset->SourceCode.empty())
            return false;

        const SplitShaderSources splitSources = SplitCombinedShaderSource(shaderAsset->SourceCode);
        if (!splitSources.IsValid)
        {
            HIMII_CORE_ERROR("Shader source missing vertex or fragment stage.");
            outCompiledShader = shaderAsset->CompiledShader;
            return false;
        }

        const std::string shaderName = shaderAsset->SourceFilePath.empty()
                                               ? "InlineShader"
                                               : shaderAsset->SourceFilePath.stem().string();
        Ref<Shader> compiledShader =
                Shader::Create(shaderName, splitSources.VertexSource, splitSources.FragmentSource);
        if (!compiledShader || !compiledShader->IsValid())
        {
            HIMII_CORE_ERROR("Shader compile failed: {0}", shaderName);
            outCompiledShader = shaderAsset->CompiledShader;
            return false;
        }

        shaderAsset->CompiledShader = compiledShader;
        shaderAsset->HasValidCompiledShader = true;
        outCompiledShader = compiledShader;
        HIMII_CORE_INFO("Shader compiled: {0}", shaderName);
        return true;
    }

    void ShaderCompilationService::InvalidateCompiledShader(const Ref<ShaderAsset> &shaderAsset)
    {
        if (!shaderAsset)
            return;
        shaderAsset->HasValidCompiledShader = false;
    }
}
