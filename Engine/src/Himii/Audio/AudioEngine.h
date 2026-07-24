#pragma once

#include "Himii/Core/Core.h"
#include "Himii/Audio/SoundAsset.h"

#include <cstdint>

namespace Himii
{
    /// 运行时 voice 句柄；0 表示无效。
    using AudioVoiceHandle = uint32_t;

    class AudioEngine
    {
    public:
        static constexpr uint32_t MaximumVoiceCount = 32;
        static constexpr AudioVoiceHandle InvalidVoiceHandle = 0;

        static void Init();
        static void Shutdown();
        static bool IsInitialized();

        /// 绑定到 SoundPlayer 的主轨：可 Stop/Pause；Loop 的主轨尽量不被 OneShot 挤掉。
        static AudioVoiceHandle Play(const Ref<SoundAsset>& soundAsset, float volume, bool loop,
                                     bool protectFromEviction = true);
        static AudioVoiceHandle PlayOneShot(const Ref<SoundAsset>& soundAsset, float volume);

        static void Stop(AudioVoiceHandle voiceHandle);
        static void Pause(AudioVoiceHandle voiceHandle);
        static void Resume(AudioVoiceHandle voiceHandle);
        static void SetVolume(AudioVoiceHandle voiceHandle, float volume);
        static bool IsPlaying(AudioVoiceHandle voiceHandle);

        static void StopAll();

        /// 编辑器 Inspector 预览（不占用场景主轨语义）。
        static void Preview(const Ref<SoundAsset>& soundAsset, float volume = 1.0f);
        static void SetPreviewVolume(float volume);
        static void StopPreview();

    private:
        AudioEngine() = default;
    };
}
