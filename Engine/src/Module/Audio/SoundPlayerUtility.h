#pragma once

#include "World/Scene/Components.h"
#include "World/Scene/Entity.h"

namespace Himii::SoundPlayerUtility
{
    void ResolveSoundAsset(SoundPlayerComponent& component);
    void Play(SoundPlayerComponent& component);
    void Stop(SoundPlayerComponent& component);
    void Pause(SoundPlayerComponent& component);
    void Resume(SoundPlayerComponent& component);
    void PlayOneShot(SoundPlayerComponent& component, AssetHandle oneShotSoundHandle);
    void ApplyVolume(SoundPlayerComponent& component);
    void StopAllPlayersInScene(class Scene* scene);
}
