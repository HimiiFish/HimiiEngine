#pragma once

#include "Module/Render/Shader/ShaderPropertyTypes.h"
#include "Module/Render/RenderCore/Shader.h"
#include "Resource/Asset.h"
#include <filesystem>
#include <vector>

namespace Himii
{
    class ShaderAsset : public Asset
    {
    public:
        AssetType GetType() const override { return AssetType::Shader; }

        ShaderPipelineType PipelineType = ShaderPipelineType::SpatialLit;
        std::vector<ShaderPropertyDefinition> PropertyDefinitions;
        std::string SourceCode;
        std::filesystem::path SourceFilePath;

        bool IsBuiltin = false;
        bool HasValidCompiledShader = false;
        Ref<Shader> CompiledShader;

        const ShaderPropertyDefinition *FindPropertyDefinition(const std::string &name) const;
    };
}
