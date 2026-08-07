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

        if (dialogState.AwaitingMaterialReimportChoice)
        {
            if (!ImGui::IsPopupOpen("Reimport Companion Materials"))
                ImGui::OpenPopup("Reimport Companion Materials");

            bool confirmed = false;
            if (ImGui::BeginPopupModal("Reimport Companion Materials", &dialogState.Open,
                                       ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted(
                        "Existing companion materials were found for this mesh.");
                ImGui::TextUnformatted("Overwrite imported materials or keep your edits?");

                if (ImGui::Button("Overwrite Materials", ImVec2(180.0f, 0.0f)))
                {
                    dialogState.PreserveCompanionMaterialsOnReimport = false;
                    dialogState.AwaitingMaterialReimportChoice = false;
                    dialogState.Open = false;
                    confirmed = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Keep Materials", ImVec2(180.0f, 0.0f)))
                {
                    dialogState.PreserveCompanionMaterialsOnReimport = true;
                    dialogState.AwaitingMaterialReimportChoice = false;
                    dialogState.Open = false;
                    confirmed = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
                {
                    dialogState.AwaitingMaterialReimportChoice = false;
                    ImGui::CloseCurrentPopup();
                    ImGui::OpenPopup("Import Static Mesh");
                }

                ImGui::EndPopup();
            }

            return confirmed;
        }

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
                const bool needsMaterialChoice =
                        dialogState.IsReimport
                        && dialogState.Settings.ImportMaterialsAndTextures
                        && dialogState.HasExistingCompanionMaterials;

                if (needsMaterialChoice)
                {
                    dialogState.AwaitingMaterialReimportChoice = true;
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    dialogState.PreserveCompanionMaterialsOnReimport = false;
                    confirmed = true;
                    dialogState.Open = false;
                    ImGui::CloseCurrentPopup();
                }
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
