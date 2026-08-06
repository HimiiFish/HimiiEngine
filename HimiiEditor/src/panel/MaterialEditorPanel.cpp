#include "MaterialEditorPanel.h"
#include "InspectorControls.h"

#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Project/Project.h"
#include "Resource/AssetManager.h"
#include "Resource/ResourceSystem.h"

#include <imgui.h>

namespace Himii
{
    void MaterialEditorPanel::SetMaterialHandle(AssetHandle materialHandle)
    {
        m_MaterialHandle = materialHandle;
        m_IsDirty = false;
        ReloadMaterial();
    }

    void MaterialEditorPanel::ReloadMaterial()
    {
        m_MaterialAsset = nullptr;
        m_AlbedoPreviewTexture = nullptr;
        if (m_MaterialHandle == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(m_MaterialHandle))
            return;

        Ref<Asset> materialBase = assetManager->GetAsset(m_MaterialHandle);
        if (!materialBase || materialBase->GetType() != AssetType::Material)
            return;

        m_MaterialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);

        if (m_MaterialAsset->AlbedoTextureHandle != 0)
        {
            Ref<Asset> textureBase = assetManager->GetAsset(m_MaterialAsset->AlbedoTextureHandle);
            if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                m_AlbedoPreviewTexture = std::static_pointer_cast<Texture2D>(textureBase);
        }
    }

    bool MaterialEditorPanel::SaveActiveMaterialAsset()
    {
        if (!m_MaterialAsset || m_MaterialHandle == 0 || !Project::GetActive())
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(m_MaterialHandle);
        if (iterator == registry.end())
            return false;

        const std::filesystem::path absolutePath =
                Project::GetAssetFileSystemPath(iterator->second.FilePath);
        if (m_MaterialAsset->AlbedoTextureHandle != 0)
        {
            const auto textureIterator = registry.find(m_MaterialAsset->AlbedoTextureHandle);
            if (textureIterator != registry.end())
                m_MaterialAsset->AlbedoTextureRelativePath = textureIterator->second.FilePath.generic_string();
        }
        MaterialAssetSerializer::Serialize(absolutePath, m_MaterialAsset);
        m_IsDirty = false;
        assetManager->SerializeAssetRegistry();
        return true;
    }

    void MaterialEditorPanel::DrawMaterialProperties()
    {
        if (!m_MaterialAsset)
        {
            ImGui::TextUnformatted("No material selected.");
            return;
        }

        BeginInspectorPropertiesStyle();

        const char *shadingModeLabels[] = {"Lit", "Unlit"};
        int shadingModeIndex = static_cast<int>(m_MaterialAsset->ShadingMode);
        DrawEnumComboControl(
                "Shading Mode", shadingModeIndex, shadingModeLabels, 2,
                [&](int newIndex)
                {
                    m_MaterialAsset->ShadingMode = static_cast<MaterialShadingMode>(newIndex);
                    m_IsDirty = true;
                });

        glm::vec4 albedoColor = m_MaterialAsset->AlbedoColor;
        DrawColorControl("Albedo Color", albedoColor, glm::vec4(1.0f));
        if (albedoColor != m_MaterialAsset->AlbedoColor)
        {
            m_MaterialAsset->AlbedoColor = albedoColor;
            m_IsDirty = true;
        }

        const std::string textureDisplayName =
                m_MaterialAsset->AlbedoTextureHandle != 0
                        ? [&]() -> std::string
                        {
                            auto assetManager = ResourceSystem::GetAssetManager();
                            if (!assetManager)
                                return "Texture";
                            const auto &registry = assetManager->GetAssetRegistry();
                            auto iterator = registry.find(m_MaterialAsset->AlbedoTextureHandle);
                            if (iterator == registry.end())
                                return "Missing Texture";
                            return iterator->second.FilePath.filename().string();
                        }()
                        : "None (drag texture)";

        DrawObjectReferenceField(
                "Albedo Texture", textureDisplayName.c_str(), m_MaterialAsset->AlbedoTextureHandle != 0,
                m_AlbedoPreviewTexture,
                [&]()
                {
                    m_MaterialAsset->AlbedoTextureHandle = 0;
                    m_MaterialAsset->AlbedoTextureRelativePath.clear();
                    m_AlbedoPreviewTexture = nullptr;
                    m_IsDirty = true;
                },
                [&](const ImGuiPayload *payload)
                {
                    Ref<Texture2D> assignedTexture;
                    AssetHandle assignedHandle = m_MaterialAsset->AlbedoTextureHandle;
                    if (!AssignTextureFromContentBrowserPayload(payload, assignedTexture, assignedHandle))
                        return false;
                    m_MaterialAsset->AlbedoTextureHandle = assignedHandle;
                    const wchar_t *relativePathWide = static_cast<const wchar_t *>(payload->Data);
                    m_MaterialAsset->AlbedoTextureRelativePath =
                            std::filesystem::path(relativePathWide).generic_string();
                    m_AlbedoPreviewTexture = assignedTexture;
                    m_IsDirty = true;
                    return true;
                });

        float specular = m_MaterialAsset->Specular;
        DrawFloatControl("Specular", specular, 0.01f, 0.0f, 1.0f, nullptr, nullptr, true, 0.5f);
        if (specular != m_MaterialAsset->Specular)
        {
            m_MaterialAsset->Specular = specular;
            m_IsDirty = true;
        }

        float shininess = m_MaterialAsset->Shininess;
        DrawFloatControl("Shininess", shininess, 0.5f, 1.0f, 256.0f, nullptr, nullptr, true, 32.0f);
        if (shininess != m_MaterialAsset->Shininess)
        {
            m_MaterialAsset->Shininess = shininess;
            m_IsDirty = true;
        }

        DrawActionButtonRow("Save", [&]()
                            {
                                if (ImGui::Button("Save Material", ImVec2(160.0f, 0.0f)))
                                    SaveActiveMaterialAsset();
                                if (m_IsDirty)
                                {
                                    ImGui::SameLine();
                                    ImGui::TextUnformatted("(unsaved)");
                                }
                            });

        EndInspectorPropertiesStyle();
    }

    void MaterialEditorPanel::OnImGuiRender(bool &isOpen)
    {
        if (!isOpen)
            return;

        if (ImGui::Begin("Material Editor", &isOpen))
        {
            DrawMaterialProperties();
        }
        ImGui::End();

        if (!isOpen && m_IsDirty)
            SaveActiveMaterialAsset();
    }
}
