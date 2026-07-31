#pragma once

#include "Resource/Asset.h"
#include "EngineCore/Core/Core.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Himii
{
    class SoundAsset : public Asset
    {
    public:
        SoundAsset() = default;
        explicit SoundAsset(const std::filesystem::path& filePath);
        ~SoundAsset() override = default;

        AssetType GetType() const override { return AssetType::SoundAsset; }

        bool IsValid() const { return !m_PulseCodeModulationSamples.empty() && m_FrameCount > 0; }

        const std::filesystem::path& GetFilePath() const { return m_FilePath; }
        const std::vector<float>& GetPulseCodeModulationSamples() const { return m_PulseCodeModulationSamples; }
        uint64_t GetFrameCount() const { return m_FrameCount; }
        uint32_t GetChannelCount() const { return m_ChannelCount; }
        uint32_t GetSampleRate() const { return m_SampleRate; }

    private:
        bool DecodeFile(const std::filesystem::path& filePath);

        std::filesystem::path m_FilePath;
        std::vector<float> m_PulseCodeModulationSamples;
        uint64_t m_FrameCount = 0;
        uint32_t m_ChannelCount = 0;
        uint32_t m_SampleRate = 0;
    };
}
