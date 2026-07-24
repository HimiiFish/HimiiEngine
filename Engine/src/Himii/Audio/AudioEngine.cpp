#include "Hepch.h"
#include "Himii/Audio/AudioEngine.h"
#include "Himii/Core/Log.h"

#include "miniaudio.h"

#include <array>
#include <mutex>

namespace Himii
{
    namespace
    {
        struct VoiceSlot
        {
            bool InUse = false;
            bool IsOneShot = false;
            bool ProtectFromEviction = false;
            ma_audio_buffer AudioBuffer{};
            ma_sound Sound{};
            Ref<SoundAsset> OwningSoundAsset; // 保持 PCM 内存存活
        };

        struct AudioEngineState
        {
            bool Initialized = false;
            ma_engine Engine{};
            std::array<VoiceSlot, AudioEngine::MaximumVoiceCount> Voices{};
            VoiceSlot PreviewVoice{};
            bool PreviewActive = false;
            std::mutex Mutex;
        };

        AudioEngineState& GetState()
        {
            static AudioEngineState state;
            return state;
        }

        void UninitializeVoiceSlot(VoiceSlot& slot)
        {
            if (!slot.InUse)
                return;

            ma_sound_uninit(&slot.Sound);
            ma_audio_buffer_uninit(&slot.AudioBuffer);
            slot.OwningSoundAsset.reset();
            slot.InUse = false;
            slot.IsOneShot = false;
            slot.ProtectFromEviction = false;
        }

        bool InitializeVoiceFromSoundAsset(VoiceSlot& slot, const Ref<SoundAsset>& soundAsset, float volume,
                                           bool loop, ma_engine* engine)
        {
            if (!soundAsset || !soundAsset->IsValid() || !engine)
                return false;

            ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init(
                ma_format_f32,
                soundAsset->GetChannelCount(),
                soundAsset->GetFrameCount(),
                soundAsset->GetPulseCodeModulationSamples().data(),
                nullptr);

            if (ma_audio_buffer_init(&bufferConfig, &slot.AudioBuffer) != MA_SUCCESS)
                return false;

            const ma_uint32 flags = MA_SOUND_FLAG_NO_SPATIALIZATION;
            if (ma_sound_init_from_data_source(engine, &slot.AudioBuffer, flags, nullptr, &slot.Sound) != MA_SUCCESS)
            {
                ma_audio_buffer_uninit(&slot.AudioBuffer);
                return false;
            }

            ma_sound_set_volume(&slot.Sound, volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume));
            ma_sound_set_looping(&slot.Sound, loop ? MA_TRUE : MA_FALSE);
            if (ma_sound_start(&slot.Sound) != MA_SUCCESS)
            {
                ma_sound_uninit(&slot.Sound);
                ma_audio_buffer_uninit(&slot.AudioBuffer);
                return false;
            }

            slot.OwningSoundAsset = soundAsset;
            slot.InUse = true;
            return true;
        }

        void ReclaimFinishedOneShots(AudioEngineState& state)
        {
            for (auto& slot : state.Voices)
            {
                if (!slot.InUse || !slot.IsOneShot)
                    continue;
                if (!ma_sound_is_playing(&slot.Sound))
                    UninitializeVoiceSlot(slot);
            }
        }

        int FindFreeVoiceIndex(AudioEngineState& state, bool allowEvictOneShot)
        {
            ReclaimFinishedOneShots(state);

            for (size_t index = 0; index < state.Voices.size(); ++index)
            {
                if (!state.Voices[index].InUse)
                    return static_cast<int>(index);
            }

            if (!allowEvictOneShot)
                return -1;

            // 超限：挤掉最旧的未保护 OneShot（数组序近似）
            for (size_t index = 0; index < state.Voices.size(); ++index)
            {
                auto& slot = state.Voices[index];
                if (slot.InUse && slot.IsOneShot && !slot.ProtectFromEviction)
                {
                    UninitializeVoiceSlot(slot);
                    return static_cast<int>(index);
                }
            }

            return -1;
        }

        AudioVoiceHandle IndexToHandle(int index)
        {
            if (index < 0)
                return AudioEngine::InvalidVoiceHandle;
            return static_cast<AudioVoiceHandle>(index + 1);
        }

        int HandleToIndex(AudioVoiceHandle handle)
        {
            if (handle == AudioEngine::InvalidVoiceHandle)
                return -1;
            return static_cast<int>(handle) - 1;
        }
    }

    void AudioEngine::Init()
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (state.Initialized)
            return;

        ma_engine_config engineConfig = ma_engine_config_init();
        if (ma_engine_init(&engineConfig, &state.Engine) != MA_SUCCESS)
        {
            HIMII_CORE_ERROR("AudioEngine: failed to initialize miniaudio engine");
            return;
        }

        state.Initialized = true;
        HIMII_CORE_INFO("AudioEngine: initialized (max voices {0})", MaximumVoiceCount);
    }

    void AudioEngine::Shutdown()
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.Initialized)
            return;

        if (state.PreviewActive)
        {
            UninitializeVoiceSlot(state.PreviewVoice);
            state.PreviewActive = false;
        }

        for (auto& slot : state.Voices)
            UninitializeVoiceSlot(slot);

        ma_engine_uninit(&state.Engine);
        state.Initialized = false;
        HIMII_CORE_INFO("AudioEngine: shutdown");
    }

    bool AudioEngine::IsInitialized()
    {
        return GetState().Initialized;
    }

    AudioVoiceHandle AudioEngine::Play(const Ref<SoundAsset>& soundAsset, float volume, bool loop,
                                       bool protectFromEviction)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.Initialized)
            return InvalidVoiceHandle;

        const int voiceIndex = FindFreeVoiceIndex(state, true);
        if (voiceIndex < 0)
        {
            HIMII_CORE_WARNING("AudioEngine: no free voice for Play");
            return InvalidVoiceHandle;
        }

        VoiceSlot& slot = state.Voices[static_cast<size_t>(voiceIndex)];
        if (!InitializeVoiceFromSoundAsset(slot, soundAsset, volume, loop, &state.Engine))
        {
            HIMII_CORE_WARNING("AudioEngine: failed to start voice");
            return InvalidVoiceHandle;
        }

        slot.IsOneShot = false;
        slot.ProtectFromEviction = protectFromEviction || loop;
        return IndexToHandle(voiceIndex);
    }

    AudioVoiceHandle AudioEngine::PlayOneShot(const Ref<SoundAsset>& soundAsset, float volume)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.Initialized)
            return InvalidVoiceHandle;

        const int voiceIndex = FindFreeVoiceIndex(state, true);
        if (voiceIndex < 0)
        {
            HIMII_CORE_WARNING("AudioEngine: no free voice for PlayOneShot (limit {0})", MaximumVoiceCount);
            return InvalidVoiceHandle;
        }

        VoiceSlot& slot = state.Voices[static_cast<size_t>(voiceIndex)];
        if (!InitializeVoiceFromSoundAsset(slot, soundAsset, volume, false, &state.Engine))
            return InvalidVoiceHandle;

        slot.IsOneShot = true;
        slot.ProtectFromEviction = false;
        return IndexToHandle(voiceIndex);
    }

    void AudioEngine::Stop(AudioVoiceHandle voiceHandle)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        const int index = HandleToIndex(voiceHandle);
        if (index < 0 || index >= static_cast<int>(state.Voices.size()))
            return;
        UninitializeVoiceSlot(state.Voices[static_cast<size_t>(index)]);
    }

    void AudioEngine::Pause(AudioVoiceHandle voiceHandle)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        const int index = HandleToIndex(voiceHandle);
        if (index < 0 || index >= static_cast<int>(state.Voices.size()))
            return;
        auto& slot = state.Voices[static_cast<size_t>(index)];
        if (slot.InUse)
            ma_sound_stop(&slot.Sound);
    }

    void AudioEngine::Resume(AudioVoiceHandle voiceHandle)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        const int index = HandleToIndex(voiceHandle);
        if (index < 0 || index >= static_cast<int>(state.Voices.size()))
            return;
        auto& slot = state.Voices[static_cast<size_t>(index)];
        if (slot.InUse)
            ma_sound_start(&slot.Sound);
    }

    void AudioEngine::SetVolume(AudioVoiceHandle voiceHandle, float volume)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        const int index = HandleToIndex(voiceHandle);
        if (index < 0 || index >= static_cast<int>(state.Voices.size()))
            return;
        auto& slot = state.Voices[static_cast<size_t>(index)];
        if (slot.InUse)
            ma_sound_set_volume(&slot.Sound, volume);
    }

    bool AudioEngine::IsPlaying(AudioVoiceHandle voiceHandle)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        const int index = HandleToIndex(voiceHandle);
        if (index < 0 || index >= static_cast<int>(state.Voices.size()))
            return false;
        auto& slot = state.Voices[static_cast<size_t>(index)];
        return slot.InUse && ma_sound_is_playing(&slot.Sound);
    }

    void AudioEngine::StopAll()
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        for (auto& slot : state.Voices)
            UninitializeVoiceSlot(slot);
        if (state.PreviewActive)
        {
            UninitializeVoiceSlot(state.PreviewVoice);
            state.PreviewActive = false;
        }
    }

    void AudioEngine::Preview(const Ref<SoundAsset>& soundAsset, float volume)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.Initialized)
            return;

        if (state.PreviewActive)
        {
            UninitializeVoiceSlot(state.PreviewVoice);
            state.PreviewActive = false;
        }

        if (!InitializeVoiceFromSoundAsset(state.PreviewVoice, soundAsset, volume, false, &state.Engine))
            return;

        state.PreviewVoice.IsOneShot = true;
        state.PreviewVoice.ProtectFromEviction = false;
        state.PreviewActive = true;
    }

    void AudioEngine::SetPreviewVolume(float volume)
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.Initialized || !state.PreviewActive)
            return;
        ma_sound_set_volume(&state.PreviewVoice.Sound, volume);
    }

    void AudioEngine::StopPreview()
    {
        auto& state = GetState();
        std::lock_guard<std::mutex> lock(state.Mutex);
        if (!state.PreviewActive)
            return;
        UninitializeVoiceSlot(state.PreviewVoice);
        state.PreviewActive = false;
    }
}
