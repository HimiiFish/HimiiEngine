#include "Hepch.h"
#include "Module/Render/Renderer/FontRegressionTests.h"
#include "Module/Render/Renderer/TextLayout.h"
#include "Module/Render/RenderCore/Shader.h"
#include "EngineCore/Core/Log.h"

namespace Himii::FontRegression
{
    // 纯 CPU 冒烟测试：不依赖 OpenGL，可在编辑器启动时调用。
    bool RunTextLayoutSmokeTests()
    {
        const std::vector<char32_t> decoded = TextShaper::DecodeUtf8(u8"A中文\nB");
        if (decoded.size() != 5)
        {
            HIMII_CORE_ERROR("FontRegression: UTF-8 decode size mismatch ({0})", decoded.size());
            return false;
        }
        if (decoded[0] != U'A' || decoded[1] != U'中' || decoded[3] != U'\n')
        {
            HIMII_CORE_ERROR("FontRegression: UTF-8 decode content mismatch");
            return false;
        }

        const auto opportunities = LineBreakIterator::BuildOpportunities(decoded);
        if (opportunities.empty())
        {
            HIMII_CORE_ERROR("FontRegression: line break opportunities empty");
            return false;
        }

        HIMII_CORE_INFO("FontRegression: TextLayout smoke tests passed");
        return true;
    }

    bool RunShaderValiditySmokeTest(const Ref<Shader> &textShader)
    {
        if (!textShader || !textShader->IsValid())
        {
            HIMII_CORE_ERROR("FontRegression: Text shader is invalid");
            return false;
        }
        HIMII_CORE_INFO("FontRegression: Text shader link smoke test passed");
        return true;
    }
}
