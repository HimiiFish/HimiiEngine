#pragma once

#include <string>

namespace Himii
{
    enum class ScriptIDEType
    {
        Inherit = 0,
        VisualStudio,
        VSCode,
        Rider,
        Custom
    };

    struct ScriptIDEConfig
    {
        ScriptIDEType Type = ScriptIDEType::VisualStudio;
        std::string CustomExecutable;
        std::string CustomArguments = "{Solution}";
    };

    const char* ScriptIDETypeToString(ScriptIDEType type);
    ScriptIDEType ScriptIDETypeFromString(const std::string& str);
}
