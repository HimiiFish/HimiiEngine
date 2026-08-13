#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Module/Render/Environment/EnvironmentLightingSystem.h"
#include "Module/Render/Environment/EnvironmentMapAsset.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"

#include <imgui.h>
#include <string>

namespace Himii
{
    static std::string ResolveEnvironmentMapDisplayName(AssetHandle handle)
    {
        if (handle == 0)
            return "None (drag .hdr)";

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(handle))
            return "Missing Asset";

        const auto &registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(handle);
        if (iterator == registry.end())
            return "Missing Asset";

        return iterator->second.FilePath.filename().string();
    }

    static bool AssignEnvironmentMapFromContentBrowserPayload(const ImGuiPayload *payload,
                                                              AssetHandle &outHandle)
    {
        if (!payload || !payload->Data || payload->DataSize <= 0)
            return false;

        const std::string relativePath(static_cast<const char *>(payload->Data),
                                       static_cast<size_t>(payload->DataSize));
        std::filesystem::path path(relativePath);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (extension != ".hdr")
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        AssetHandle handle = assetManager->FindAssetHandleByFilePath(path);
        if (handle == 0)
            handle = assetManager->ImportAsset(path);
        if (handle == 0)
            return false;

        Ref<Asset> asset = assetManager->GetAsset(handle);
        if (!asset || asset->GetType() != AssetType::EnvironmentMap)
            return false;

        outHandle = handle;
        EnvironmentLightingSystem::Invalidate(handle);
        return true;
    }

    static void DrawEnvironmentComponentInspectorUI(ComponentInspectorDrawContext &drawContext)
    {
        if (!drawContext.entity.HasComponent<EnvironmentComponent>())
            return;

        auto &component = drawContext.entity.GetComponent<EnvironmentComponent>();

        DrawComponentInspectorHeaderUI(
                drawContext, "EnvironmentComponent", "Environment", nullptr,
                [&]()
                {
                    DrawCheckboxControl("Enabled", component.Enabled, true);

                    const std::string environmentDisplayName =
                            ResolveEnvironmentMapDisplayName(component.EnvironmentMap);
                    DrawObjectReferenceField(
                            "Environment Map", environmentDisplayName.c_str(),
                            component.EnvironmentMap != 0, nullptr,
                            [&]()
                            {
                                if (component.EnvironmentMap != 0)
                                    EnvironmentLightingSystem::Invalidate(component.EnvironmentMap);
                                component.EnvironmentMap = 0;
                            },
                            [&](const ImGuiPayload *payload)
                            {
                                return AssignEnvironmentMapFromContentBrowserPayload(
                                        payload, component.EnvironmentMap);
                            });

                    DrawFloatControl("Intensity", component.Intensity, 0.01f, 0.0f, 0.0f, nullptr, nullptr, true,
                                     1.0f);
                    DrawColorControl("Ambient Color", component.AmbientColor, glm::vec4(1.0f));
                    DrawFloatControl("Ambient Intensity", component.AmbientIntensity, 0.01f, 0.0f, 0.0f, nullptr,
                                     nullptr, true, 0.15f);

                    DrawReadOnlyTextControl(
                            "Note", "IBL preferred",
                            "When Environment Map bakes successfully, IBL replaces constant Ambient. First enabled Environment wins.");
                },
                [&]() { drawContext.entity.RemoveComponent<EnvironmentComponent>(); });
    }

    struct EnvironmentComponentInspectorRegistrar
    {
        EnvironmentComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<EnvironmentComponent>(
                    "EnvironmentComponent", "Environment", "Rendering", 46,
                    &DrawEnvironmentComponentInspectorUI);
        }
    };

    static EnvironmentComponentInspectorRegistrar s_EnvironmentComponentInspectorRegistrar;
}
