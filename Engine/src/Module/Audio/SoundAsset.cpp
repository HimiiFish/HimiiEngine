#include "Hepch.h"
#include "Module/Audio/SoundAsset.h"
#include "EngineCore/Core/Log.h"

#include "miniaudio.h"

#include <fstream>

namespace Himii
{
    SoundAsset::SoundAsset(const std::filesystem::path& filePath)
    {
        DecodeFile(filePath);
    }

    bool SoundAsset::DecodeFile(const std::filesystem::path& filePath)
    {
        m_FilePath = filePath;
        m_PulseCodeModulationSamples.clear();
        m_FrameCount = 0;
        m_ChannelCount = 0;
        m_SampleRate = 0;

        // Read via filesystem::path so Unicode paths work on Windows (miniaudio's narrow fopen does not).
        std::ifstream inputStream(filePath, std::ios::binary | std::ios::ate);
        if (!inputStream)
        {
            HIMII_CORE_ERROR("SoundAsset: failed to open '{0}'", filePath.string());
            return false;
        }

        const std::streamsize fileSize = inputStream.tellg();
        if (fileSize <= 0)
        {
            HIMII_CORE_ERROR("SoundAsset: empty or unreadable file '{0}'", filePath.string());
            return false;
        }

        inputStream.seekg(0, std::ios::beg);
        std::vector<uint8_t> fileBytes(static_cast<size_t>(fileSize));
        if (!inputStream.read(reinterpret_cast<char*>(fileBytes.data()), fileSize))
        {
            HIMII_CORE_ERROR("SoundAsset: failed reading bytes from '{0}'", filePath.string());
            return false;
        }

        ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
        ma_decoder decoder;
        ma_result result = ma_decoder_init_memory(fileBytes.data(), fileBytes.size(), &decoderConfig, &decoder);
        if (result != MA_SUCCESS)
        {
            HIMII_CORE_ERROR("SoundAsset: failed to decode '{0}' (miniaudio result {1}: {2})", filePath.string(),
                             static_cast<int>(result), ma_result_description(result));
            return false;
        }

        ma_uint64 frameCount = 0;
        result = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
        if (result != MA_SUCCESS || frameCount == 0)
        {
            // 部分格式无法预知长度：读到 EOF
            std::vector<float> growableBuffer;
            growableBuffer.reserve(48000 * 2);
            float readBuffer[4096];
            for (;;)
            {
                ma_uint64 framesRead = 0;
                result = ma_decoder_read_pcm_frames(&decoder, readBuffer,
                                                   (sizeof(readBuffer) / sizeof(float)) / decoder.outputChannels,
                                                   &framesRead);
                if (framesRead == 0)
                    break;
                const size_t sampleCount = static_cast<size_t>(framesRead) * decoder.outputChannels;
                growableBuffer.insert(growableBuffer.end(), readBuffer, readBuffer + sampleCount);
                frameCount += framesRead;
                if (result != MA_SUCCESS)
                    break;
            }

            m_ChannelCount = decoder.outputChannels;
            m_SampleRate = decoder.outputSampleRate;
            m_FrameCount = frameCount;
            m_PulseCodeModulationSamples = std::move(growableBuffer);
            ma_decoder_uninit(&decoder);

            if (!IsValid())
            {
                HIMII_CORE_ERROR("SoundAsset: decoded zero frames from '{0}'", filePath.string());
                return false;
            }
            return true;
        }

        m_ChannelCount = decoder.outputChannels;
        m_SampleRate = decoder.outputSampleRate;
        m_FrameCount = frameCount;
        m_PulseCodeModulationSamples.resize(static_cast<size_t>(frameCount) * m_ChannelCount);

        ma_uint64 framesRead = 0;
        result = ma_decoder_read_pcm_frames(&decoder, m_PulseCodeModulationSamples.data(), frameCount, &framesRead);
        ma_decoder_uninit(&decoder);

        if (result != MA_SUCCESS && framesRead == 0)
        {
            HIMII_CORE_ERROR("SoundAsset: failed reading PCM from '{0}'", filePath.string());
            m_PulseCodeModulationSamples.clear();
            m_FrameCount = 0;
            return false;
        }

        m_FrameCount = framesRead;
        m_PulseCodeModulationSamples.resize(static_cast<size_t>(framesRead) * m_ChannelCount);
        return IsValid();
    }
}
