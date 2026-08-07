#include "Hepch.h"
#include "Module/Render/Shader/ShaderSourceUtility.h"
#include "EngineCore/Core/Log.h"

namespace Himii
{
    namespace
    {
        std::string ShaderStageNameFromTypeToken(const std::string &typeToken)
        {
            if (typeToken == "vertex")
                return "vertex";
            if (typeToken == "fragment" || typeToken == "pixel")
                return "fragment";
            return {};
        }
    }

    SplitShaderSources SplitCombinedShaderSource(const std::string &combinedSource)
    {
        SplitShaderSources splitSources;
        const char *typeToken = "#type";
        const size_t typeTokenLength = strlen(typeToken);
        size_t position = combinedSource.find(typeToken, 0);

        while (position != std::string::npos)
        {
            const size_t endOfLine = combinedSource.find_first_of("\r\n", position);
            if (endOfLine == std::string::npos)
                break;

            const size_t begin = position + typeTokenLength + 1;
            const std::string stageType = combinedSource.substr(begin, endOfLine - begin);
            const std::string normalizedStage = ShaderStageNameFromTypeToken(stageType);
            if (normalizedStage.empty())
            {
                HIMII_CORE_ERROR("Invalid shader stage type token: {0}", stageType);
                return splitSources;
            }

            const size_t nextLinePosition = combinedSource.find_first_not_of("\r\n", endOfLine);
            if (nextLinePosition == std::string::npos)
                break;

            position = combinedSource.find(typeToken, nextLinePosition);
            const std::string stageSource = combinedSource.substr(
                    nextLinePosition,
                    position == std::string::npos ? std::string::npos : position - nextLinePosition);

            if (normalizedStage == "vertex")
                splitSources.VertexSource = stageSource;
            else if (normalizedStage == "fragment")
                splitSources.FragmentSource = stageSource;
        }

        splitSources.IsValid = !splitSources.VertexSource.empty() && !splitSources.FragmentSource.empty();
        return splitSources;
    }
}
