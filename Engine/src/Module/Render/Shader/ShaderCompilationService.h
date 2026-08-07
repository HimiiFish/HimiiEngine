#pragma once

#include "Module/Render/Shader/ShaderAsset.h"
#include "EngineCore/Core/Core.h"

namespace Himii
{
    class ShaderCompilationService
    {
    public:
        static Ref<Shader> GetOrCompileShader(const Ref<ShaderAsset> &shaderAsset);
        static bool TryCompileShaderAsset(const Ref<ShaderAsset> &shaderAsset, Ref<Shader> &outCompiledShader);
        static void InvalidateCompiledShader(const Ref<ShaderAsset> &shaderAsset);
    };
}
