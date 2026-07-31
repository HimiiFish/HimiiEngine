#pragma once

#include "Module/Script/ScriptIDE.h"
#include <filesystem>

namespace Himii
{
    class EditorSettings
    {
    public:
        static EditorSettings& Get();

        ScriptIDEConfig DefaultScriptIDE;

        void Load();
        void Save() const;

    private:
        EditorSettings() = default;
        static std::filesystem::path GetSettingsPath();
    };
}
