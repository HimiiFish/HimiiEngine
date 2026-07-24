#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Himii/Asset/AssetManager.h"
#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Project/Project.h"
#include "Himii/Scene/Components.h"
#include "Himii/Scene/ParticleEmitterAsset.h"

#include <filesystem>
#include <imgui.h>

namespace Himii
{
    static std::string ResolveParticleEmitterDisplayName(AssetHandle emitterHandle)
    {
        if (emitterHandle == 0)
            return "None (drag .particle)";

        auto assetManager = Project::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(emitterHandle))
            return "Missing Asset";

        const auto& registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(emitterHandle);
        if (iterator == registry.end())
            return "Missing Asset";

        return iterator->second.FilePath.filename().string();
    }

    static void DrawParticleEmitterComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<ParticleEmitterComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<ParticleEmitterComponent>();

        DrawComponentInspectorHeaderUI(
            drawContext, "ParticleEmitterComponent", "Particle Emitter", nullptr,
            [&]()
            {
                auto assetManager = Project::GetAssetManager();
                const std::string emitterDisplayName =
                    ResolveParticleEmitterDisplayName(component.EmitterHandle);

                DrawObjectReferenceField(
                    "Emitter", emitterDisplayName.c_str(), component.EmitterHandle != 0, nullptr,
                    [&]()
                    {
                        component.EmitterHandle = 0;
                    },
                    [&](const ImGuiPayload* payload)
                    {
                        return AssignParticleEmitterAssetFromContentBrowserPayload(
                            payload, component.EmitterHandle);
                    },
                    [&]()
                    {
                        if (component.EmitterHandle != 0 && drawContext.requestParticleEmitterEditor)
                            drawContext.requestParticleEmitterEditor(component.EmitterHandle);
                    });

                if (component.EmitterHandle == 0 && assetManager)
                {
                    DrawActionButtonRow(
                        "Emitter",
                        [&]()
                        {
                            if (ImGui::Button("Create New Particle Emitter", ImVec2(-1.0f, 0.0f)))
                            {
                                auto assetDirectory = Project::GetAssetDirectory();
                                std::filesystem::path directory = assetDirectory / "particles";
                                std::filesystem::create_directories(directory);
                                std::filesystem::path path = directory / "new_emitter.particle";
                                int nameIndex = 0;
                                while (std::filesystem::exists(path))
                                    path = directory
                                        / ("new_emitter_" + std::to_string(++nameIndex) + ".particle");

                                auto emitterAsset = std::make_shared<ParticleEmitterAsset>();
                                emitterAsset->Handle = AssetHandle();
                                ParticleEmitterAssetSerializer::Serialize(path, emitterAsset);
                                auto relativePath = std::filesystem::relative(path, assetDirectory);
                                AssetHandle handle = assetManager->ImportAsset(relativePath);
                                if (handle != 0)
                                {
                                    component.EmitterHandle = handle;
                                    emitterAsset->Handle = handle;
                                    ParticleEmitterAssetSerializer::Serialize(path, emitterAsset);
                                    assetManager->SerializeAssetRegistry();
                                }
                            }
                        });
                }
            },
            [&]() { drawContext.entity.RemoveComponent<ParticleEmitterComponent>(); });
    }

    struct ParticleEmitterComponentInspectorRegistrar
    {
        ParticleEmitterComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<ParticleEmitterComponent>(
                "ParticleEmitterComponent", "Particle Emitter", "", 120,
                &DrawParticleEmitterComponentInspectorUI);
        }
    };

    static ParticleEmitterComponentInspectorRegistrar s_ParticleEmitterComponentInspectorRegistrar;
}
