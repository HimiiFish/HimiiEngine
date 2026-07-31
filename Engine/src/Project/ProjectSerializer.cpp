#include "Hepch.h"
#include "ProjectSerializer.h"
#include "Module/Script/ScriptIDE.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Himii
{

    ProjectSerializer::ProjectSerializer(Ref<Project> project) : m_Project(project)
    {
    }

    bool ProjectSerializer::Serialize(const std::filesystem::path &filepath, bool omitScriptIDEFields)
    {
        const auto &config = m_Project->m_Config;

        YAML::Emitter out;
        {
            out << YAML::BeginMap; // Root
            out << YAML::Key << "Project" << YAML::Value;
            {
                out << YAML::BeginMap; // Project
                out << YAML::Key << "Name" << YAML::Value << config.Name;
                out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.string();
                out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
                out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
                out << YAML::Key << "Is2D" << YAML::Value << config.Is2D;
                if (!omitScriptIDEFields)
                {
                    out << YAML::Key << "ScriptIDE" << YAML::Value << ScriptIDETypeToString(config.ScriptIDEOverride);
                    out << YAML::Key << "CustomScriptIDEPath" << YAML::Value << config.CustomScriptIDEPath;
                    out << YAML::Key << "CustomScriptIDEArguments" << YAML::Value << config.CustomScriptIDEArguments;
                }

                out << YAML::Key << "Physics2DLayers" << YAML::Value;
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "LayerNames" << YAML::Value << YAML::BeginSeq;
                    for (int layerIndex = 0; layerIndex < Physics2DLayerCount; ++layerIndex)
                        out << YAML::Value << config.Physics2DLayers.LayerNames[layerIndex];
                    out << YAML::EndSeq;

                    out << YAML::Key << "CollisionMatrix" << YAML::Value << YAML::BeginSeq;
                    for (int row = 0; row < Physics2DLayerCount; ++row)
                    {
                        out << YAML::BeginSeq;
                        for (int column = 0; column < Physics2DLayerCount; ++column)
                            out << YAML::Value << config.Physics2DLayers.CollisionMatrix[row][column];
                        out << YAML::EndSeq;
                    }
                    out << YAML::EndSeq;
                    out << YAML::EndMap;
                }

                out << YAML::Key << "SortingLayers" << YAML::Value;
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << "LayerNames" << YAML::Value << YAML::BeginSeq;
                    for (int layerIndex = 0; layerIndex < SortingLayerCount; ++layerIndex)
                        out << YAML::Value << config.SortingLayers.LayerNames[layerIndex];
                    out << YAML::EndSeq;
                    out << YAML::EndMap;
                }

                out << YAML::EndMap; // Project
            }
            out << YAML::EndMap; // Root
        }

        std::ofstream outputStream(filepath, std::ios::binary | std::ios::trunc);
        if (!outputStream)
        {
            HIMII_CORE_ERROR("Failed to open project file for write: {0}", filepath.string());
            return false;
        }

        outputStream << out.c_str();
        if (!outputStream)
        {
            HIMII_CORE_ERROR("Failed to write project file: {0}", filepath.string());
            return false;
        }

        return true;
    }

    bool ProjectSerializer::Deserialize(const std::filesystem::path &filepath)
    {
        auto &config = m_Project->m_Config;

        YAML::Node data;
        try
        {
            data = YAML::LoadFile(filepath.string());
        }
        catch (YAML::ParserException e)
        {
            HIMII_CORE_ERROR("Failed to load project file '{0}'\n{1}", filepath.string(), e.what());
            return false;
        }

        auto projectNode = data["Project"];
        if (!projectNode)
            return false;

        config.Name = projectNode["Name"].as<std::string>();
        config.StartScene = projectNode["StartScene"].as<std::string>();
        config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
        config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();
        
        if (projectNode["Is2D"])
             config.Is2D = projectNode["Is2D"].as<bool>();

        if (projectNode["ScriptIDE"])
            config.ScriptIDEOverride = ScriptIDETypeFromString(projectNode["ScriptIDE"].as<std::string>());
        else
            config.ScriptIDEOverride = ScriptIDEType::Inherit;

        if (projectNode["CustomScriptIDEPath"])
            config.CustomScriptIDEPath = projectNode["CustomScriptIDEPath"].as<std::string>();

        if (projectNode["CustomScriptIDEArguments"])
            config.CustomScriptIDEArguments = projectNode["CustomScriptIDEArguments"].as<std::string>();

        if (auto physicsLayersNode = projectNode["Physics2DLayers"])
        {
            if (auto layerNamesNode = physicsLayersNode["LayerNames"])
            {
                for (size_t layerIndex = 0;
                     layerIndex < layerNamesNode.size() && layerIndex < Physics2DLayerCount;
                     ++layerIndex)
                {
                    config.Physics2DLayers.LayerNames[layerIndex] = layerNamesNode[layerIndex].as<std::string>();
                }
            }

            if (auto collisionMatrixNode = physicsLayersNode["CollisionMatrix"])
            {
                for (size_t row = 0; row < collisionMatrixNode.size() && row < Physics2DLayerCount; ++row)
                {
                    const auto rowNode = collisionMatrixNode[row];
                    for (size_t column = 0; column < rowNode.size() && column < Physics2DLayerCount; ++column)
                        config.Physics2DLayers.CollisionMatrix[row][column] = rowNode[column].as<bool>();
                }
            }
        }

        if (auto sortingLayersNode = projectNode["SortingLayers"])
        {
            if (auto layerNamesNode = sortingLayersNode["LayerNames"])
            {
                for (size_t layerIndex = 0;
                     layerIndex < layerNamesNode.size() && layerIndex < SortingLayerCount;
                     ++layerIndex)
                {
                    config.SortingLayers.LayerNames[layerIndex] = layerNamesNode[layerIndex].as<std::string>();
                }
            }
        }

        return true;
    }
}
