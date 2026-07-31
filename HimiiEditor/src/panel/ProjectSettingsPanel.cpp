#include "Hepch.h"
#include "ProjectSettingsPanel.h"
#include "Project/Project.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "Module/Script/ScriptIDE.h"

#include <imgui.h>
#include <cstring>
#include <cstdio>

namespace Himii
{
    static const char* kProjectIDELabels[] = { "Use Editor Default", "Visual Studio", "VS Code", "Rider", "Custom" };
    static const ScriptIDEType kProjectIDEValues[] = {
        ScriptIDEType::Inherit, ScriptIDEType::VisualStudio, ScriptIDEType::VSCode,
        ScriptIDEType::Rider, ScriptIDEType::Custom
    };

    void ProjectSettingsPanel::SyncFromProject()
    {
        if (!Project::GetActive())
            return;

        const auto& config = Project::GetConfig();
        m_TempIDEOverride = config.ScriptIDEOverride;
        strncpy(m_TempCustomPath, config.CustomScriptIDEPath.c_str(), sizeof(m_TempCustomPath) - 1);
        strncpy(m_TempCustomArgs, config.CustomScriptIDEArguments.c_str(), sizeof(m_TempCustomArgs) - 1);
        m_TempPhysics2DLayers = config.Physics2DLayers;
        m_TempSortingLayers = config.SortingLayers;
    }

    void ProjectSettingsPanel::ApplyToProject()
    {
        if (!Project::GetActive())
            return;

        auto& config = Project::GetConfig();
        config.ScriptIDEOverride = m_TempIDEOverride;
        config.CustomScriptIDEPath = m_TempCustomPath;
        config.CustomScriptIDEArguments = m_TempCustomArgs;
        config.Physics2DLayers = m_TempPhysics2DLayers;
        config.SortingLayers = m_TempSortingLayers;
    }

    void ProjectSettingsPanel::OnImGuiRender(bool* pOpen)
    {
        if (!Project::GetActive())
            return;

        if (ImGui::IsWindowAppearing() || !m_Initialized)
        {
            SyncFromProject();
            m_Initialized = true;
        }

        ImGui::SetNextWindowSize(ImVec2(560, 640), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Project Settings", pOpen))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Script IDE (Project Override)");
        ImGui::Separator();

        int currentType = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (kProjectIDEValues[i] == m_TempIDEOverride)
            {
                currentType = i;
                break;
            }
        }

        if (ImGui::Combo("Script IDE", &currentType, kProjectIDELabels, 5))
            m_TempIDEOverride = kProjectIDEValues[currentType];

        if (m_TempIDEOverride == ScriptIDEType::Custom)
        {
            ImGui::InputText("Executable", m_TempCustomPath, sizeof(m_TempCustomPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse##ProjIDE"))
            {
                std::string path = FileDialog::OpenFile("Executable (*.exe)\0*.exe\0");
                if (!path.empty())
                    strncpy(m_TempCustomPath, path.c_str(), sizeof(m_TempCustomPath) - 1);
            }

            ImGui::InputText("Arguments", m_TempCustomArgs, sizeof(m_TempCustomArgs));
            ImGui::TextDisabled("Leave empty to use editor default arguments.");
            ImGui::TextDisabled("Placeholders: {ProjectDir} {Solution} {File} {ProjectName}");
        }

        ImGui::Separator();
        ImGui::Text("Physics 2D Layers");
        ImGui::Separator();

        for (int layerIndex = 0; layerIndex < Physics2DLayerCount; ++layerIndex)
        {
            ImGui::PushID(layerIndex);
            char layerNameBuffer[64];
            strncpy(layerNameBuffer, m_TempPhysics2DLayers.LayerNames[layerIndex].c_str(), sizeof(layerNameBuffer) - 1);
            layerNameBuffer[sizeof(layerNameBuffer) - 1] = '\0';

            ImGui::Text("Layer %d", layerIndex);
            ImGui::SameLine(80.0f);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##LayerName", layerNameBuffer, sizeof(layerNameBuffer)))
                m_TempPhysics2DLayers.LayerNames[layerIndex] = layerNameBuffer;
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Text("Collision Matrix");
        if (ImGui::BeginTable("Physics2DCollisionMatrix", Physics2DLayerCount + 1,
                               ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Layer", ImGuiTableColumnFlags_WidthFixed, 72.0f);
            for (int column = 0; column < Physics2DLayerCount; ++column)
            {
                char columnHeader[16];
                snprintf(columnHeader, sizeof(columnHeader), "%d", column);
                ImGui::TableSetupColumn(columnHeader, ImGuiTableColumnFlags_WidthFixed, 24.0f);
            }
            ImGui::TableHeadersRow();

            for (int row = 0; row < Physics2DLayerCount; ++row)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", row);

                for (int column = 0; column < Physics2DLayerCount; ++column)
                {
                    ImGui::TableSetColumnIndex(column + 1);
                    ImGui::PushID(row * Physics2DLayerCount + column);
                    bool canCollide = m_TempPhysics2DLayers.CollisionMatrix[row][column];
                    if (ImGui::Checkbox("##Collide", &canCollide))
                    {
                        m_TempPhysics2DLayers.CollisionMatrix[row][column] = canCollide;
                        m_TempPhysics2DLayers.CollisionMatrix[column][row] = canCollide;
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Text("Sorting Layers");
        ImGui::TextDisabled("Lower index draws first (behind). Reorder by renaming only; index is the layer ID.");
        ImGui::Separator();

        for (int layerIndex = 0; layerIndex < SortingLayerCount; ++layerIndex)
        {
            ImGui::PushID(1000 + layerIndex);
            char layerNameBuffer[64];
            strncpy(layerNameBuffer, m_TempSortingLayers.LayerNames[layerIndex].c_str(), sizeof(layerNameBuffer) - 1);
            layerNameBuffer[sizeof(layerNameBuffer) - 1] = '\0';

            ImGui::Text("Layer %d", layerIndex);
            ImGui::SameLine(80.0f);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##SortingLayerName", layerNameBuffer, sizeof(layerNameBuffer)))
                m_TempSortingLayers.LayerNames[layerIndex] = layerNameBuffer;
            ImGui::PopID();
        }

        ImGui::Separator();
        if (ImGui::Button("Apply"))
            ApplyToProject();

        ImGui::SameLine();
        ImGui::TextDisabled("(Use File -> Save Project to persist)");

        ImGui::End();
    }
}
