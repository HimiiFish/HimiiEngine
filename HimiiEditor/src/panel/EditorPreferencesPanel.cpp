#include "Hepch.h"
#include "EditorPreferencesPanel.h"
#include "EngineCore/Editor/EditorSettings.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "Module/Script/ScriptIDE.h"

#include <imgui.h>
#include <cstring>

namespace Himii
{
    static const char* kIDETypeLabels[] = { "Visual Studio", "VS Code", "Rider", "Custom" };
    static const ScriptIDEType kIDETypeValues[] = {
        ScriptIDEType::VisualStudio, ScriptIDEType::VSCode, ScriptIDEType::Rider, ScriptIDEType::Custom
    };

    void EditorPreferencesPanel::DrawIDEConfig(ScriptIDEConfig& config, const char* idSuffix)
    {
        int currentType = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (kIDETypeValues[i] == config.Type)
            {
                currentType = i;
                break;
            }
        }

        std::string comboLabel = std::string("IDE Type##") + idSuffix;
        if (ImGui::Combo(comboLabel.c_str(), &currentType, kIDETypeLabels, 4))
            config.Type = kIDETypeValues[currentType];

        if (config.Type == ScriptIDEType::Custom)
        {
            static char exePath[512] = "";
            static char args[512] = "";
            static bool initialized = false;

            if (!initialized || ImGui::IsWindowAppearing())
            {
                strncpy(exePath, config.CustomExecutable.c_str(), sizeof(exePath) - 1);
                strncpy(args, config.CustomArguments.c_str(), sizeof(args) - 1);
                initialized = true;
            }

            std::string pathLabel = std::string("Executable##") + idSuffix;
            ImGui::InputText(pathLabel.c_str(), exePath, sizeof(exePath));
            ImGui::SameLine();
            std::string browseLabel = std::string("Browse##") + idSuffix;
            if (ImGui::Button(browseLabel.c_str()))
            {
                std::string path = FileDialog::OpenFile("Executable (*.exe)\0*.exe\0");
                if (!path.empty())
                    strncpy(exePath, path.c_str(), sizeof(exePath) - 1);
            }

            std::string argsLabel = std::string("Arguments##") + idSuffix;
            ImGui::InputText(argsLabel.c_str(), args, sizeof(args));
            ImGui::TextDisabled("Placeholders: {ProjectDir} {Solution} {File} {ProjectName} {Line}");

            config.CustomExecutable = exePath;
            config.CustomArguments = args;
        }
    }

    void EditorPreferencesPanel::OnImGuiRender(bool* pOpen)
    {
        ImGui::SetNextWindowSize(ImVec2(480, 260), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Editor Preferences", pOpen))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Default Script IDE");
        ImGui::Separator();

        auto& settings = EditorSettings::Get();
        DrawIDEConfig(settings.DefaultScriptIDE, "Global");

        ImGui::Separator();
        if (ImGui::Button("Save"))
        {
            settings.Save();
            ImGui::CloseCurrentPopup();
        }

        ImGui::End();
    }
}
