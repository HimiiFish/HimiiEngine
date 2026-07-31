#include "Hepch.h"
#include "ScriptIDE.h"

namespace Himii
{
    const char* ScriptIDETypeToString(ScriptIDEType type)
    {
        switch (type)
        {
            case ScriptIDEType::Inherit: return "Inherit";
            case ScriptIDEType::VisualStudio: return "VisualStudio";
            case ScriptIDEType::VSCode: return "VSCode";
            case ScriptIDEType::Rider: return "Rider";
            case ScriptIDEType::Custom: return "Custom";
            default: return "VisualStudio";
        }
    }

    ScriptIDEType ScriptIDETypeFromString(const std::string& str)
    {
        if (str == "Inherit") return ScriptIDEType::Inherit;
        if (str == "VisualStudio") return ScriptIDEType::VisualStudio;
        if (str == "VSCode") return ScriptIDEType::VSCode;
        if (str == "Rider") return ScriptIDEType::Rider;
        if (str == "Custom") return ScriptIDEType::Custom;
        return ScriptIDEType::VisualStudio;
    }
}
