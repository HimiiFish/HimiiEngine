#include "Hepch.h"
#include "panel/StaticMeshImportDialog.h"
#include "InspectorControls.h"

#include <imgui.h>

namespace Himii
{
    bool DrawStaticMeshImportDialog(StaticMeshImportDialogState &dialogState)
    {
        if (!dialogState.Open)
            return false;

        bool confirmed = false;
        if (ImGui::BeginPopupModal("Import Static Mesh", &dialogState.Open,
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(dialogState.IsReimport ? "Reimport static mesh with settings:"
                                                          : "Import static mesh with settings:");

            DrawFloatControl("Uniform Scale", dialogState.Settings.UniformScale, 0.01f, 0.001f, 1000.0f,
                             nullptr, nullptr, true, 1.0f);

            bool importMaterials = dialogState.Settings.ImportMaterialsAndTextures;
            if (ImGui::Checkbox("Import Materials & Textures", &importMaterials))
                dialogState.Settings.ImportMaterialsAndTextures = importMaterials;

            bool combineMeshes = dialogState.Settings.CombineMeshes;
            if (ImGui::Checkbox("Combine Meshes", &combineMeshes))
                dialogState.Settings.CombineMeshes = combineMeshes;

            if (ImGui::Button("Import", ImVec2(120.0f, 0.0f)))
            {
                confirmed = true;
                dialogState.Open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            {
                dialogState.Open = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return confirmed;
    }
}
