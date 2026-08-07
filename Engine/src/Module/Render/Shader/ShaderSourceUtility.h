#pragma once

#include "EngineCore/Core/Core.h"
#include <string>
#include <utility>

namespace Himii
{
    struct SplitShaderSources
    {
        std::string VertexSource;
        std::string FragmentSource;
        bool IsValid = false;
    };

    SplitShaderSources SplitCombinedShaderSource(const std::string &combinedSource);
}
