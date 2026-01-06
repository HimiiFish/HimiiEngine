#pragma once

#include <vector>
#include "Himii/Asset/Asset.h"

namespace Himii
{

    class SpriteAnimation : public Asset {
    public:
        SpriteAnimation() = default;
        virtual ~SpriteAnimation() = default;

        virtual AssetType GetType() const override
        {
            return AssetType::SpriteAnimation;
        }

        void AddFrame(AssetHandle textureHandle)
        {
            m_Frames.push_back(textureHandle);
        }

        const std::vector<AssetHandle> &GetFrames() const
        {
            return m_Frames;
        }

        // 获取特定帧的 AssetHandle，如果索引越界返回 0
        AssetHandle GetFrame(int index) const
        {
            if (index >= 0 && index < m_Frames.size())
                return m_Frames[index];
            return 0;
        }

        size_t GetFrameCount() const
        {
            return m_Frames.size();
        }

    private:
        std::vector<AssetHandle> m_Frames;
    };
} // namespace Himii
