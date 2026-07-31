#include "Hepch.h"
#include "EditorSettings.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{
    EditorSettings& EditorSettings::Get()
    {
        static EditorSettings instance;
        return instance;
    }

    std::filesystem::path EditorSettings::GetSettingsPath()
    {
        return std::filesystem::current_path() / "editor_settings.yaml";
    }

    void EditorSettings::Load()
    {
        auto path = GetSettingsPath();
        if (!std::filesystem::exists(path))
            return;

        try
        {
            YAML::Node data = YAML::LoadFile(path.string());
            auto editorNode = data["Editor"];
            if (!editorNode)
                return;

            if (editorNode["DefaultScriptIDE"])
                DefaultScriptIDE.Type = ScriptIDETypeFromString(editorNode["DefaultScriptIDE"].as<std::string>());

            if (editorNode["CustomScriptIDEPath"])
                DefaultScriptIDE.CustomExecutable = editorNode["CustomScriptIDEPath"].as<std::string>();

            if (editorNode["CustomScriptIDEArguments"])
                DefaultScriptIDE.CustomArguments = editorNode["CustomScriptIDEArguments"].as<std::string>();
        }
        catch (const YAML::Exception& e)
        {
            HIMII_CORE_ERROR("Failed to load editor settings: {0}", e.what());
        }
    }

    void EditorSettings::Save() const
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Editor" << YAML::Value;
        {
            out << YAML::BeginMap;
            out << YAML::Key << "DefaultScriptIDE" << YAML::Value << ScriptIDETypeToString(DefaultScriptIDE.Type);
            out << YAML::Key << "CustomScriptIDEPath" << YAML::Value << DefaultScriptIDE.CustomExecutable;
            out << YAML::Key << "CustomScriptIDEArguments" << YAML::Value << DefaultScriptIDE.CustomArguments;
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        std::ofstream fout(GetSettingsPath());
        fout << out.c_str();
    }
}
