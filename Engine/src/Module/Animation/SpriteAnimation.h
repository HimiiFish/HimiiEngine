#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Resource/Asset.h"

namespace Himii
{

    enum class SpriteAnimationLoopMode
    {
        Loop = 0,
        Once,
        PingPong
    };

    inline constexpr const char* SpriteAnimationDefaultClipName = "default";

    struct SpriteAnimationClip
    {
        std::string Name = SpriteAnimationDefaultClipName;
        float FrameRate = 10.0f;
        SpriteAnimationLoopMode LoopMode = SpriteAnimationLoopMode::Loop;
        std::vector<AssetHandle> Frames;
        std::vector<glm::ivec2> AtlasFrameCoordinates;

        bool UsesAtlasFrames(AssetHandle sharedAtlasTextureHandle) const
        {
            return sharedAtlasTextureHandle != 0 && !AtlasFrameCoordinates.empty();
        }

        bool UsesFrameList(AssetHandle sharedAtlasTextureHandle) const
        {
            return !Frames.empty() && !UsesAtlasFrames(sharedAtlasTextureHandle);
        }

        size_t GetFrameCount(AssetHandle sharedAtlasTextureHandle) const
        {
            if (UsesAtlasFrames(sharedAtlasTextureHandle))
                return AtlasFrameCoordinates.size();
            return Frames.size();
        }

        AssetHandle GetFrame(int frameIndex, AssetHandle sharedAtlasTextureHandle) const
        {
            if (frameIndex >= 0 && frameIndex < static_cast<int>(Frames.size()))
                return Frames[static_cast<size_t>(frameIndex)];
            return 0;
        }

        glm::ivec2 GetAtlasFrameCoordinates(int frameIndex) const
        {
            if (frameIndex >= 0 && frameIndex < static_cast<int>(AtlasFrameCoordinates.size()))
                return AtlasFrameCoordinates[static_cast<size_t>(frameIndex)];
            return {0, 0};
        }

        void AddAtlasFrame(const glm::ivec2& atlasCoordinates)
        {
            AtlasFrameCoordinates.push_back(atlasCoordinates);
            Frames.clear();
        }

        void RemoveAtlasFrameAt(int frameIndex)
        {
            if (frameIndex >= 0 && frameIndex < static_cast<int>(AtlasFrameCoordinates.size()))
                AtlasFrameCoordinates.erase(AtlasFrameCoordinates.begin() + frameIndex);
        }

        void SetAtlasFrameCoordinatesAt(int frameIndex, const glm::ivec2& atlasCoordinates)
        {
            if (frameIndex >= 0 && frameIndex < static_cast<int>(AtlasFrameCoordinates.size()))
                AtlasFrameCoordinates[static_cast<size_t>(frameIndex)] = atlasCoordinates;
        }

        void MoveAtlasFrame(int frameIndex, int direction)
        {
            const int newIndex = frameIndex + direction;
            if (frameIndex < 0 || newIndex < 0
                || frameIndex >= static_cast<int>(AtlasFrameCoordinates.size())
                || newIndex >= static_cast<int>(AtlasFrameCoordinates.size()))
                return;

            std::swap(AtlasFrameCoordinates[static_cast<size_t>(frameIndex)],
                      AtlasFrameCoordinates[static_cast<size_t>(newIndex)]);
        }

        void AddFrame(AssetHandle frameHandle)
        {
            Frames.push_back(frameHandle);
            AtlasFrameCoordinates.clear();
        }

        void RemoveFrameAt(int frameIndex)
        {
            if (frameIndex >= 0 && frameIndex < static_cast<int>(Frames.size()))
                Frames.erase(Frames.begin() + frameIndex);
        }

        void ClearFrames()
        {
            Frames.clear();
            AtlasFrameCoordinates.clear();
        }

        void GenerateAtlasGridFrames(uint32_t columnCount, uint32_t rowCount)
        {
            AtlasFrameCoordinates.clear();
            Frames.clear();
            for (uint32_t row = 0; row < rowCount; ++row)
            {
                for (uint32_t column = 0; column < columnCount; ++column)
                    AtlasFrameCoordinates.push_back({static_cast<int>(column), static_cast<int>(row)});
            }
        }
    };

    class SpriteAnimation : public Asset {
    public:
        SpriteAnimation() = default;
        virtual ~SpriteAnimation() = default;

        virtual AssetType GetType() const override
        {
            return AssetType::SpriteAnimation;
        }

        AssetHandle GetAtlasTextureHandle() const { return m_AtlasTextureHandle; }
        uint32_t GetAtlasGridCellSize() const { return m_AtlasGridCellSize; }

        bool UsesAtlasTexture() const
        {
            return m_AtlasTextureHandle != 0;
        }

        void SetAtlasTexture(AssetHandle textureHandle, uint32_t gridCellSize)
        {
            m_AtlasTextureHandle = textureHandle;
            m_AtlasGridCellSize = gridCellSize > 0 ? gridCellSize : 16;
        }

        void SetAtlasGridCellSize(uint32_t gridCellSize)
        {
            m_AtlasGridCellSize = gridCellSize > 0 ? gridCellSize : 16;
        }

        void ClearAtlasMode()
        {
            m_AtlasTextureHandle = 0;
            for (SpriteAnimationClip& clip : m_NamedAnimations)
                clip.AtlasFrameCoordinates.clear();
        }

        const std::vector<SpriteAnimationClip>& GetNamedAnimations() const
        {
            return m_NamedAnimations;
        }

        std::vector<SpriteAnimationClip>& GetNamedAnimationsMutable()
        {
            return m_NamedAnimations;
        }

        std::vector<std::string> GetAnimationNames() const
        {
            std::vector<std::string> names;
            names.reserve(m_NamedAnimations.size());
            for (const SpriteAnimationClip& clip : m_NamedAnimations)
                names.push_back(clip.Name);
            return names;
        }

        bool HasAnimation(const std::string& animationName) const
        {
            return FindClip(animationName) != nullptr;
        }

        const SpriteAnimationClip* FindClip(const std::string& animationName) const
        {
            for (const SpriteAnimationClip& clip : m_NamedAnimations)
            {
                if (clip.Name == animationName)
                    return &clip;
            }
            return nullptr;
        }

        SpriteAnimationClip* FindClipMutable(const std::string& animationName)
        {
            for (SpriteAnimationClip& clip : m_NamedAnimations)
            {
                if (clip.Name == animationName)
                    return &clip;
            }
            return nullptr;
        }

        SpriteAnimationClip& EnsureClip(const std::string& animationName)
        {
            if (SpriteAnimationClip* existingClip = FindClipMutable(animationName))
                return *existingClip;

            SpriteAnimationClip newClip;
            newClip.Name = animationName;
            m_NamedAnimations.push_back(newClip);
            return m_NamedAnimations.back();
        }

        bool RemoveClip(const std::string& animationName)
        {
            for (auto iterator = m_NamedAnimations.begin(); iterator != m_NamedAnimations.end(); ++iterator)
            {
                if (iterator->Name == animationName)
                {
                    m_NamedAnimations.erase(iterator);
                    return true;
                }
            }
            return false;
        }

        bool RenameClip(const std::string& oldName, const std::string& newName)
        {
            if (oldName == newName || newName.empty() || HasAnimation(newName))
                return false;

            if (SpriteAnimationClip* clip = FindClipMutable(oldName))
            {
                clip->Name = newName;
                return true;
            }
            return false;
        }

        const SpriteAnimationClip* GetPrimaryClip() const
        {
            if (const SpriteAnimationClip* defaultClip = FindClip(SpriteAnimationDefaultClipName))
                return defaultClip;
            if (m_NamedAnimations.empty())
                return nullptr;
            return &m_NamedAnimations.front();
        }

        size_t GetFrameCount(const std::string& animationName) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->GetFrameCount(m_AtlasTextureHandle);
            return 0;
        }

        bool UsesAtlasFrames(const std::string& animationName) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->UsesAtlasFrames(m_AtlasTextureHandle);
            return false;
        }

        bool UsesFrameList(const std::string& animationName) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->UsesFrameList(m_AtlasTextureHandle);
            return false;
        }

        float GetClipFrameRate(const std::string& animationName) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->FrameRate;
            return 10.0f;
        }

        SpriteAnimationLoopMode GetClipLoopMode(const std::string& animationName) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->LoopMode;
            return SpriteAnimationLoopMode::Loop;
        }

        glm::ivec2 GetAtlasFrameCoordinates(const std::string& animationName, int frameIndex) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->GetAtlasFrameCoordinates(frameIndex);
            return {0, 0};
        }

        AssetHandle GetFrame(const std::string& animationName, int frameIndex) const
        {
            if (const SpriteAnimationClip* clip = FindClip(animationName))
                return clip->GetFrame(frameIndex, m_AtlasTextureHandle);
            return 0;
        }

        void MigrateLegacySingleAnimation(float defaultFrameRate, SpriteAnimationLoopMode loopMode,
                                          const std::vector<AssetHandle>& frames,
                                          const std::vector<glm::ivec2>& atlasCoordinates)
        {
            m_NamedAnimations.clear();
            SpriteAnimationClip clip;
            clip.Name = SpriteAnimationDefaultClipName;
            clip.FrameRate = defaultFrameRate;
            clip.LoopMode = loopMode;
            clip.Frames = frames;
            clip.AtlasFrameCoordinates = atlasCoordinates;
            m_NamedAnimations.push_back(clip);
        }

    private:
        AssetHandle m_AtlasTextureHandle = 0;
        uint32_t m_AtlasGridCellSize = 16;
        std::vector<SpriteAnimationClip> m_NamedAnimations;
    };

} // namespace Himii
