#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Module/Render/Mesh/MeshAsset.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"

#include <imgui.h>
#include <string>

namespace Himii
{
    static std::string ResolveAssetDisplayName(AssetHandle handle, const char *emptyHint)
    {
        if (handle == 0)
            return emptyHint;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(handle))
            return "Missing Asset";

        const auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(handle);
        if (iterator == registry.end())
            return "Missing Asset";

        return iterator->second.FilePath.filename().string();
    }

    static void ApplyDefaultMaterialsFromMesh(MeshComponent &component)
    {
        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || component.MeshAssetHandle == 0)
            return;

        Ref<Asset> meshBase = assetManager->GetAsset(component.MeshAssetHandle);
        if (!meshBase || meshBase->GetType() != AssetType::Mesh)
            return;

        Ref<MeshAsset> meshAsset = std::static_pointer_cast<MeshAsset>(meshBase);
        component.MaterialAssetHandles = meshAsset->DefaultMaterialHandles;
        NormalizeMeshComponentMaterialSlots(component);
    }

    static void DrawMaterialSlot(ComponentInspectorDrawContext &drawContext, MeshComponent &component,
                                 size_t materialIndex, const std::string &slotLabel)
    {
        if (materialIndex >= component.MaterialAssetHandles.size())
            return;

        AssetHandle &materialHandle = component.MaterialAssetHandles[materialIndex];
        const std::string materialDisplayName =
                ResolveAssetDisplayName(materialHandle, "None (drag .hmaterial)");
        DrawObjectReferenceField(
                slotLabel.c_str(), materialDisplayName.c_str(), materialHandle != 0, nullptr,
                [&]() { materialHandle = 0; },
                [&](const ImGuiPayload *payload)
                {
                    return AssignMaterialAssetFromContentBrowserPayload(payload, materialHandle);
                },
                [&]()
                {
                    if (materialHandle != 0 && drawContext.requestMaterialEditor)
                        drawContext.requestMaterialEditor(materialHandle);
                });
    }

    static void DrawMeshMaterialSlots(ComponentInspectorDrawContext &drawContext, MeshComponent &component)
    {
        NormalizeMeshComponentMaterialSlots(component);

        if (component.Source == MeshComponent::MeshSource::Builtin)
            DrawReadOnlyTextControl("Shading", "Lit", "Builtin meshes always render with the Lit pipeline.");

        auto assetManager = ResourceSystem::GetAssetManager();
        Ref<MeshAsset> meshAssetForSlots;
        if (component.Source == MeshComponent::MeshSource::Asset && assetManager
            && component.MeshAssetHandle != 0)
        {
            Ref<Asset> meshBase = assetManager->GetAsset(component.MeshAssetHandle);
            if (meshBase && meshBase->GetType() == AssetType::Mesh)
                meshAssetForSlots = std::static_pointer_cast<MeshAsset>(meshBase);
        }

        for (size_t materialIndex = 0; materialIndex < component.MaterialAssetHandles.size();
             ++materialIndex)
        {
            std::string slotLabel;
            if (component.Source == MeshComponent::MeshSource::Builtin)
                slotLabel = "Material 0";
            else if (meshAssetForSlots && materialIndex < meshAssetForSlots->MaterialSlotNames.size()
                     && !meshAssetForSlots->MaterialSlotNames[materialIndex].empty())
                slotLabel = meshAssetForSlots->MaterialSlotNames[materialIndex];
            else
                slotLabel = "Slot " + std::to_string(materialIndex);

            DrawMaterialSlot(drawContext, component, materialIndex, slotLabel);
        }
    }

    static void DrawMeshComponentInspectorUI(ComponentInspectorDrawContext &drawContext)
    {
        if (!drawContext.entity.HasComponent<MeshComponent>())
            return;

        auto &component = drawContext.entity.GetComponent<MeshComponent>();
        Ref<Texture2D> icon =
                drawContext.getComponentIcon ? drawContext.getComponentIcon("Mesh Renderer") : nullptr;

        DrawComponentInspectorHeaderUI(
                drawContext, "MeshComponent", "Mesh Renderer", icon,
                [&]()
                {
                    const char *sourceStrings[] = {"Builtin", "Asset"};
                    int sourceIndex = static_cast<int>(component.Source);
                    DrawEnumComboControl(
                            "Source", sourceIndex, sourceStrings, 2,
                            [&](int newIndex)
                            {
                                component.Source = static_cast<MeshComponent::MeshSource>(newIndex);
                                NormalizeMeshComponentMaterialSlots(component);
                            });

                    if (component.Source == MeshComponent::MeshSource::Builtin)
                    {
                        const char *meshTypeStrings[] = {"Cube", "Plane", "Sphere", "Capsule"};
                        int meshTypeIndex = static_cast<int>(component.Type);
                        DrawEnumComboControl(
                                "Mesh Type", meshTypeIndex, meshTypeStrings, 4,
                                [&](int newIndex)
                                {
                                    component.Type = static_cast<MeshComponent::MeshType>(newIndex);
                                });
                    }
                    else
                    {
                        const std::string meshDisplayName =
                                ResolveAssetDisplayName(component.MeshAssetHandle, "None (drag .hmesh)");
                        DrawObjectReferenceField(
                                "Mesh", meshDisplayName.c_str(), component.MeshAssetHandle != 0, nullptr,
                                [&]()
                                {
                                    component.MeshAssetHandle = 0;
                                    NormalizeMeshComponentMaterialSlots(component);
                                },
                                [&](const ImGuiPayload *payload)
                                {
                                    if (!AssignMeshAssetFromContentBrowserPayload(payload,
                                                                                  component.MeshAssetHandle))
                                        return false;
                                    ApplyDefaultMaterialsFromMesh(component);
                                    return true;
                                });
                    }

                    DrawMeshMaterialSlots(drawContext, component);
                },
                [&]() { drawContext.entity.RemoveComponent<MeshComponent>(); });
    }

    struct MeshComponentInspectorRegistrar
    {
        MeshComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<MeshComponent>(
                    "MeshComponent", "Mesh Renderer", "Mesh Renderer", 40, &DrawMeshComponentInspectorUI);
        }
    };

    static MeshComponentInspectorRegistrar s_MeshComponentInspectorRegistrar;
}
