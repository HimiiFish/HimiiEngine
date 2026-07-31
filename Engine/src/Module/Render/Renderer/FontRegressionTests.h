#pragma once

#include "EngineCore/Core/Core.h"
#include "Module/Render/RenderCore/Shader.h"

namespace Himii::FontRegression
{
    bool RunTextLayoutSmokeTests();
    bool RunShaderValiditySmokeTest(const Ref<Shader> &textShader);
}
