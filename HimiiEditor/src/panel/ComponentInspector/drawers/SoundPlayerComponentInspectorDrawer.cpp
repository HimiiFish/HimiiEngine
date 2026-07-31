#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Resource/AssetManager.h"
#include "Module/Audio/AudioEngine.h"
#include "Module/Audio/SoundPlayerUtility.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"

#include <filesystem>
#include <imgui.h>

namespace Himii
{
    static std::string ResolveAssetDisplayName(AssetHandle assetHandle, const char* emptyLabel)
    {
        if (assetHandle == 0)
            return emptyLabel;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(assetHandle))
            return "Missing Asset";

        const auto& registry = assetManager->GetAssetRegistry();
        auto iterator = registry.find(assetHandle);
        if (iterator == registry.end())
            return "Missing Asset";

        return iterator->second.FilePath.filename().string();
    }

    static void DrawSoundPlayerComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<SoundPlayerComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<SoundPlayerComponent>();

        DrawComponentInspectorHeaderUI(
            drawContext, "SoundPlayerComponent", "Sound Player", nullptr,
            [&]()
            {
                const std::string soundDisplayName =
                    ResolveAssetDisplayName(component.SoundHandle, "None (drag .wav/.ogg/.mp3)");

                DrawObjectReferenceField(
                    "Sound", soundDisplayName.c_str(), component.SoundHandle != 0, nullptr,
                    [&]()
                    {
                        component.SoundHandle = 0;
                        component.Sound = nullptr;
                        SoundPlayerUtility::Stop(component);
                    },
                    [&](const ImGuiPayload* payload)
                    {
                        if (!AssignSoundAssetFromContentBrowserPayload(payload, component.SoundHandle))
                            return false;
                        SoundPlayerUtility::ResolveSoundAsset(component);
                        return true;
                    });

                float volume = component.Volume;
                DrawFloatControl("Volume", volume, 0.01f, 0.0f, 1.0f, nullptr, nullptr, true, 1.0f);
                if (volume != component.Volume)
                {
                    component.Volume = volume;
                    SoundPlayerUtility::ApplyVolume(component);
                }

                bool previousMute = component.Mute;
                DrawCheckboxControl("Mute", component.Mute, false);
                if (component.Mute != previousMute)
                    SoundPlayerUtility::ApplyVolume(component);

                DrawCheckboxControl("Loop", component.Loop, false);
                DrawCheckboxControl("Play On Start", component.PlayOnStart, false);

                DrawActionButtonRow("Preview", [&]()
                {
                    if (ImGui::Button("Preview", ImVec2(120.0f, 0.0f)))
                    {
                        SoundPlayerUtility::ResolveSoundAsset(component);
                        if (component.Sound)
                            AudioEngine::Preview(component.Sound, component.EvaluateEffectiveVolume());
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop Preview", ImVec2(120.0f, 0.0f)))
                        AudioEngine::StopPreview();
                });
            },
            [&]()
            {
                SoundPlayerUtility::Stop(component);
                drawContext.entity.RemoveComponent<SoundPlayerComponent>();
            });
    }

    struct SoundPlayerComponentInspectorRegistrar
    {
        SoundPlayerComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<SoundPlayerComponent>(
                "SoundPlayerComponent", "Sound Player", "Audio", 260,
                &DrawSoundPlayerComponentInspectorUI);
        }
    };

    static SoundPlayerComponentInspectorRegistrar s_SoundPlayerComponentInspectorRegistrar;
}
