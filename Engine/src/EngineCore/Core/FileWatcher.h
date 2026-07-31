#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace Himii
{
    class FileWatcher
    {
    public:
        using ChangeCallback = std::function<void()>;

        void Watch(const std::filesystem::path& directory, ChangeCallback onChanged, float debounceSeconds = 0.5f);
        void WatchFile(const std::filesystem::path& filePath, ChangeCallback onChanged, float debounceSeconds = 0.5f);
        void Clear();
        void Update(float deltaSeconds);

        bool HasPendingChange() const { return m_PendingChange; }
        void ClearPendingChange() { m_PendingChange = false; }

    private:
        void ScanDirectory();

        std::filesystem::path m_WatchPath;
        bool m_WatchSingleFile = false;
        ChangeCallback m_Callback;
        float m_DebounceSeconds = 0.5f;
        float m_DebounceTimer = 0.0f;
        bool m_PendingChange = false;
        bool m_Dirty = false;
        float m_ScanAccumulator = 0.0f;
        static constexpr float s_ScanInterval = 0.25f;

        std::unordered_map<std::string, std::filesystem::file_time_type> m_FileTimes;
        bool m_Initialized = false;
    };
}
