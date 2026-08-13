#include "MaterialEditorPanel.h"
#include "InspectorControls.h"
#include "panel/MaterialThumbnailUtility.h"

#include "Module/Render/Mesh/MaterialAssetSerializer.h"
#include "Module/Render/Mesh/MaterialSurfaceUtility.h"
#include "Module/Render/Shader/BuiltinShaderRegistry.h"
#include "Module/Render/Shader/ShaderAsset.h"
#include "Project/Project.h"
#include "Resource/AssetManager.h"
#include "Resource/ResourceSystem.h"

#include <imgui.h>

namespace Himii
{
    bool MaterialEditorPanel::IsActiveMaterialDirty() const
    {
        return m_MaterialHandle != 0
               && m_DirtyMaterialHandles.find(m_MaterialHandle) != m_DirtyMaterialHandles.end();
    }

    void MaterialEditorPanel::MarkActiveMaterialDirty()
    {
        if (m_MaterialHandle != 0)
            m_DirtyMaterialHandles.insert(m_MaterialHandle);
    }

    void MaterialEditorPanel::SetMaterialHandle(AssetHandle materialHandle)
    {
        if (materialHandle == m_MaterialHandle)
            return;

        if (m_MaterialHandle != 0 && IsActiveMaterialDirty())
            SaveMaterialAsset(m_MaterialHandle);

        m_MaterialHandle = materialHandle;
        ReloadMaterial();
    }

    void MaterialEditorPanel::ReloadMaterial()
    {
        m_MaterialAsset = nullptr;
        m_ShaderAsset = nullptr;
        if (m_MaterialHandle == 0)
            return;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(m_MaterialHandle))
            return;

        Ref<Asset> materialBase = assetManager->GetAsset(m_MaterialHandle);
        if (!materialBase || materialBase->GetType() != AssetType::Material)
            return;

        m_MaterialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);
        m_ShaderAsset = ResolveShaderAsset(assetManager.get(), m_MaterialAsset->ShaderHandle);
    }

    bool MaterialEditorPanel::SaveMaterialAsset(AssetHandle materialHandle)
    {
        if (materialHandle == 0 || !Project::GetActive())
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(materialHandle);
        if (iterator == registry.end())
            return false;

        Ref<Asset> materialBase = assetManager->GetAsset(materialHandle);
        if (!materialBase || materialBase->GetType() != AssetType::Material)
            return false;

        Ref<MaterialAsset> materialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);
        assetManager->ResolveMaterialAlbedoTextureReference(*materialAsset);

        const std::filesystem::path absolutePath =
                Project::GetAssetFileSystemPath(iterator->second.FilePath);
        MaterialAssetSerializer::Serialize(absolutePath, materialAsset);
        m_DirtyMaterialHandles.erase(materialHandle);
        InvalidateMaterialThumbnail(materialHandle);
        return true;
    }

    int MaterialEditorPanel::SaveAllDirtyMaterialAssets()
    {
        if (!Project::GetActive() || m_DirtyMaterialHandles.empty())
            return 0;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return 0;

        const std::vector<AssetHandle> dirtyHandles(m_DirtyMaterialHandles.begin(),
                                                    m_DirtyMaterialHandles.end());
        int savedCount = 0;
        for (AssetHandle materialHandle : dirtyHandles)
        {
            if (SaveMaterialAsset(materialHandle))
                savedCount++;
        }

        if (savedCount > 0)
            assetManager->SerializeAssetRegistry();

        return savedCount;
    }

    void MaterialEditorPanel::DiscardAllDirtyMaterialChanges()
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (assetManager)
        {
            for (AssetHandle materialHandle : m_DirtyMaterialHandles)
                assetManager->UnloadAsset(materialHandle);
        }

        m_DirtyMaterialHandles.clear();
        ReloadMaterial();
    }

    static std::string ResolveShaderDisplayName(AssetHandle shaderHandle)
    {
        if (BuiltinShaderHandles::IsBuiltinShaderHandle(shaderHandle))
        {
            if (shaderHandle == BuiltinShaderHandles::MeshLit)
                return "MeshLit (Built-in)";
            if (shaderHandle == BuiltinShaderHandles::MeshUnlit)
                return "MeshUnlit (Built-in)";
        }

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(shaderHandle))
            return "Missing Shader";

        const auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(shaderHandle);
        if (iterator == registry.end())
            return "Missing Shader";
        return iterator->second.FilePath.filename().string();
    }

    void MaterialEditorPanel::DrawMaterialProperties()
    {
        if (!m_MaterialAsset)
        {
            ImGui::TextUnformatted("No material selected.");
            return;
        }

        BeginInspectorPropertiesStyle();

        DrawReadOnlyTextControl("Shader", ResolveShaderDisplayName(m_MaterialAsset->ShaderHandle).c_str(),
                                "Material instances inherit parameter layout from the referenced shader.");

        if (!m_ShaderAsset)
        {
            ImGui::TextUnformatted("Shader definition unavailable.");
            EndInspectorPropertiesStyle();
            return;
        }

        for (const ShaderPropertyDefinition &definition : m_ShaderAsset->PropertyDefinitions)
        {
            MaterialParameterValue parameterValue =
                    ResolveMaterialParameterValue(*m_MaterialAsset, definition);
            const char *label = definition.DisplayName.empty() ? definition.Name.c_str()
                                                                : definition.DisplayName.c_str();

            switch (definition.Type)
            {
                case ShaderPropertyType::Float:
                {
                    float floatValue = parameterValue.FloatValue;
                    DrawFloatControl(label, floatValue, 0.01f, 0.0f, 0.0f, nullptr, nullptr, true,
                                     definition.DefaultFloat);
                    if (floatValue != parameterValue.FloatValue)
                    {
                        m_MaterialAsset->SetFloatParameter(definition.Name, floatValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Int:
                {
                    float intAsFloat = static_cast<float>(parameterValue.IntValue);
                    DrawFloatControl(label, intAsFloat, 1.0f, 0.0f, 0.0f, nullptr, nullptr, true,
                                     static_cast<float>(definition.DefaultInt));
                    const int intValue = static_cast<int>(intAsFloat);
                    if (intValue != parameterValue.IntValue)
                    {
                        m_MaterialAsset->SetIntParameter(definition.Name, intValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Bool:
                {
                    bool boolValue = parameterValue.BoolValue;
                    DrawCheckboxControl(label, boolValue, definition.DefaultBool);
                    if (boolValue != parameterValue.BoolValue)
                    {
                        m_MaterialAsset->SetBoolParameter(definition.Name, boolValue);
                        MarkActiveMaterialDirty();
                        if (definition.Name == "u_NormalFlipGreen")
                            InvalidateMaterialThumbnail(m_MaterialHandle);
                    }
                    break;
                }
                case ShaderPropertyType::Color:
                {
                    glm::vec4 colorValue = parameterValue.ColorValue;
                    DrawColorControl(label, colorValue, definition.DefaultColor);
                    if (colorValue != parameterValue.ColorValue)
                    {
                        m_MaterialAsset->SetColorParameter(definition.Name, colorValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Vector2:
                {
                    glm::vec2 vectorValue = parameterValue.Vector2Value;
                    DrawVec2Control(label, vectorValue, 0.01f, 0.0f, 0.0f, nullptr, true,
                                    definition.DefaultVector2);
                    if (vectorValue != parameterValue.Vector2Value)
                    {
                        m_MaterialAsset->SetVector2Parameter(definition.Name, vectorValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Vector3:
                {
                    glm::vec3 vectorValue = parameterValue.Vector3Value;
                    DrawVec3Control(label, vectorValue);
                    if (vectorValue != parameterValue.Vector3Value)
                    {
                        m_MaterialAsset->SetVector3Parameter(definition.Name, vectorValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Vector4:
                {
                    glm::vec4 vectorValue = parameterValue.Vector4Value;
                    DrawColorControl(label, vectorValue, definition.DefaultVector4);
                    if (vectorValue != parameterValue.Vector4Value)
                    {
                        m_MaterialAsset->SetVector4Parameter(definition.Name, vectorValue);
                        MarkActiveMaterialDirty();
                    }
                    break;
                }
                case ShaderPropertyType::Texture2D:
                {
                    auto assetManager = ResourceSystem::GetAssetManager();
                    Ref<Texture2D> previewTexture;
                    if (parameterValue.TextureHandle != 0 && assetManager)
                    {
                        Ref<Asset> textureBase = assetManager->GetAsset(parameterValue.TextureHandle);
                        if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                            previewTexture = std::static_pointer_cast<Texture2D>(textureBase);
                    }

                    std::string textureDisplayName = "None (drag texture)";
                    if (parameterValue.TextureHandle != 0 && assetManager)
                    {
                        const auto &registry = assetManager->GetAssetRegistry();
                        auto iterator = registry.find(parameterValue.TextureHandle);
                        if (iterator != registry.end())
                            textureDisplayName = iterator->second.FilePath.filename().string();
                    }

                    DrawObjectReferenceField(
                            label, textureDisplayName.c_str(), parameterValue.TextureHandle != 0,
                            previewTexture,
                            [&]()
                            {
                                m_MaterialAsset->SetTextureParameter(definition.Name, 0, {});
                                MarkActiveMaterialDirty();
                                InvalidateMaterialThumbnail(m_MaterialHandle);
                            },
                            [&](const ImGuiPayload *payload)
                            {
                                Ref<Texture2D> assignedTexture;
                                AssetHandle assignedHandle = 0;
                                if (!AssignTextureFromContentBrowserPayload(payload, assignedTexture,
                                                                            assignedHandle))
                                    return false;

                                std::string relativePath;
                                const wchar_t *relativePathWide =
                                        static_cast<const wchar_t *>(payload->Data);
                                relativePath = std::filesystem::path(relativePathWide).generic_string();
                                m_MaterialAsset->SetTextureParameter(definition.Name, assignedHandle,
                                                                     relativePath);
                                MarkActiveMaterialDirty();
                                InvalidateMaterialThumbnail(m_MaterialHandle);
                                return true;
                            });
                    break;
                }
            }
        }

        if (IsActiveMaterialDirty())
            ImGui::TextUnformatted("(unsaved)");

        EndInspectorPropertiesStyle();
    }

    void MaterialEditorPanel::ResetForProjectChange()
    {
        m_MaterialHandle = 0;
        m_MaterialAsset = nullptr;
        m_ShaderAsset = nullptr;
        m_DirtyMaterialHandles.clear();
    }

    void MaterialEditorPanel::OnImGuiRender(bool &isOpen)
    {
        if (!isOpen)
            return;

        std::string panelTitle = "Material Editor";
        if (IsActiveMaterialDirty())
            panelTitle += "*";

        if (ImGui::Begin(panelTitle.c_str(), &isOpen))
            DrawMaterialProperties();

        ImGui::End();
    }
}
