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
    }

    static void DrawMaterialSlot(ComponentInspectorDrawContext &drawContext, MeshComponent &component,
                                 size_t materialIndex)
    {
        if (materialIndex >= component.MaterialAssetHandles.size())
            return;

        AssetHandle &materialHandle = component.MaterialAssetHandles[materialIndex];
        const std::string label = "Material " + std::to_string(materialIndex);
        const std::string materialDisplayName =
                ResolveAssetDisplayName(materialHandle, "None (drag .hmaterial)");
        DrawObjectReferenceField(
                label.c_str(), materialDisplayName.c_str(), materialHandle != 0, nullptr,
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

                        if (component.MaterialAssetHandles.empty())
                        {
                            AssetHandle materialHandle = 0;
                            const std::string materialDisplayName =
                                    ResolveAssetDisplayName(materialHandle, "None (drag .hmaterial)");
                            DrawObjectReferenceField(
                                    "Material 0", materialDisplayName.c_str(), false, nullptr,
                                    [&]() {},
                                    [&](const ImGuiPayload *payload)
                                    {
                                        AssetHandle assignedHandle = 0;
                                        if (!AssignMaterialAssetFromContentBrowserPayload(payload,
                                                                                          assignedHandle))
                                            return false;
                                        component.MaterialAssetHandles.push_back(assignedHandle);
                                        return true;
                                    });
                        }
                        else
                        {
                            DrawMaterialSlot(drawContext, component, 0);
                        }
                    }
                    else
                    {
                        const std::string meshDisplayName =
                                ResolveAssetDisplayName(component.MeshAssetHandle, "None (drag .glb/.gltf)");
                        DrawObjectReferenceField(
                                "Mesh", meshDisplayName.c_str(), component.MeshAssetHandle != 0, nullptr,
                                [&]()
                                {
                                    component.MeshAssetHandle = 0;
                                    component.MaterialAssetHandles.clear();
                                },
                                [&](const ImGuiPayload *payload)
                                {
                                    if (!AssignMeshAssetFromContentBrowserPayload(payload,
                                                                                  component.MeshAssetHandle))
                                        return false;
                                    ApplyDefaultMaterialsFromMesh(component);
                                    return true;
                                });

                        for (size_t materialIndex = 0; materialIndex < component.MaterialAssetHandles.size();
                             ++materialIndex)
                        {
                            DrawMaterialSlot(drawContext, component, materialIndex);
                        }
                    }

                    DrawColorControl("Fallback Color", component.Color);
                    DrawReadOnlyTextControl(
                            "Color Usage", "Fallback when no material",
                            "Albedo comes from Material when assigned. Fallback Color is used only when the material slot is empty.");
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
