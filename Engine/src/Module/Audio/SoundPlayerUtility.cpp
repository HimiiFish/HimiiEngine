#include "Hepch.h"
#include "Module/Audio/SoundPlayerUtility.h"
#include "Module/Audio/AudioEngine.h"
#include "Resource/AssetManager.h"
#include "Project/Project.h"
#include "World/Scene/Scene.h"

namespace Himii::SoundPlayerUtility
{
    void ResolveSoundAsset(SoundPlayerComponent& component)
    {
        if (!component.SoundHandle || !Project::GetActive())
        {
            component.Sound = nullptr;
            return;
        }

        auto assetManager = Project::GetActive()->GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(component.SoundHandle))
        {
            component.Sound = nullptr;
            return;
        }

        component.Sound = std::dynamic_pointer_cast<SoundAsset>(assetManager->GetAsset(component.SoundHandle));
    }

    void Play(SoundPlayerComponent& component)
    {
        ResolveSoundAsset(component);
        Stop(component);

        if (!component.Sound || !component.Sound->IsValid())
            return;

        component.RuntimeVoiceHandle =
                AudioEngine::Play(component.Sound, component.EvaluateEffectiveVolume(), component.Loop, true);
        component.RuntimePaused = false;
        // 再写一次音量，避免个别后端在 start 时忽略初始 gain。
        if (component.RuntimeVoiceHandle != AudioEngine::InvalidVoiceHandle)
            AudioEngine::SetVolume(component.RuntimeVoiceHandle, component.EvaluateEffectiveVolume());
    }

    void Stop(SoundPlayerComponent& component)
    {
        if (component.RuntimeVoiceHandle != AudioEngine::InvalidVoiceHandle)
        {
            AudioEngine::Stop(component.RuntimeVoiceHandle);
            component.RuntimeVoiceHandle = AudioEngine::InvalidVoiceHandle;
        }
        component.RuntimePaused = false;
    }

    void Pause(SoundPlayerComponent& component)
    {
        if (component.RuntimeVoiceHandle == AudioEngine::InvalidVoiceHandle)
            return;
        AudioEngine::Pause(component.RuntimeVoiceHandle);
        component.RuntimePaused = true;
    }

    void Resume(SoundPlayerComponent& component)
    {
        if (component.RuntimeVoiceHandle == AudioEngine::InvalidVoiceHandle)
            return;
        AudioEngine::Resume(component.RuntimeVoiceHandle);
        component.RuntimePaused = false;
    }

    void PlayOneShot(SoundPlayerComponent& component, AssetHandle oneShotSoundHandle)
    {
        Ref<SoundAsset> oneShotSound = component.Sound;
        if (oneShotSoundHandle && Project::GetActive())
        {
            auto assetManager = Project::GetActive()->GetAssetManager();
            if (assetManager && assetManager->IsAssetHandleValid(oneShotSoundHandle))
                oneShotSound = std::dynamic_pointer_cast<SoundAsset>(assetManager->GetAsset(oneShotSoundHandle));
        }

        if (!oneShotSound || !oneShotSound->IsValid())
            return;

        AudioEngine::PlayOneShot(oneShotSound, component.EvaluateEffectiveVolume());
    }

    void ApplyVolume(SoundPlayerComponent& component)
    {
        const float effectiveVolume = component.EvaluateEffectiveVolume();
        if (component.RuntimeVoiceHandle != AudioEngine::InvalidVoiceHandle)
            AudioEngine::SetVolume(component.RuntimeVoiceHandle, effectiveVolume);
        // Inspector Preview 使用独立 voice，需同步音量，否则拖动 Volume 看起来“不生效”。
        AudioEngine::SetPreviewVolume(effectiveVolume);
    }

    void StopAllPlayersInScene(Scene* scene)
    {
        if (!scene)
            return;

        auto view = scene->GetAllEntitiesWith<SoundPlayerComponent>();
        for (auto entityHandle : view)
        {
            auto& player = view.get<SoundPlayerComponent>(entityHandle);
            Stop(player);
        }
        AudioEngine::StopAll();
    }
}
