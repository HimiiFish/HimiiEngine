#include "Hepch.h"
#include "Module/Render/Shader/ShaderAsset.h"

namespace Himii
{
    const ShaderPropertyDefinition *ShaderAsset::FindPropertyDefinition(const std::string &name) const
    {
        for (const ShaderPropertyDefinition &definition : PropertyDefinitions)
        {
            if (definition.Name == name)
                return &definition;
        }
        return nullptr;
    }
}
