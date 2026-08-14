#pragma once

#include <cstdint>

#include "glm/vec3.hpp"

namespace Himii
{
    /// 引擎立方体面约定（与后端无关）：
    /// - 世界 Y 向上；面序与 TextureCube::SetFaceData 一致。
    /// - 面图像素左上为原点；侧视面行 0 为世界 +Y。
    /// 烘焙生成与 CPU 采样必须成对使用本头，禁止各写各的面映射。
    enum class CubemapFace : uint32_t
    {
        PositiveX = 0,
        NegativeX = 1,
        PositiveY = 2,
        NegativeY = 3,
        PositiveZ = 4,
        NegativeZ = 5
    };

    constexpr uint32_t CubemapFaceCount = 6;

    struct CubemapFaceSample
    {
        CubemapFace Face = CubemapFace::PositiveX;
        /// 0 为左，1 为右。
        float ImageU = 0.0f;
        /// 0 为上，1 为下。
        float ImageV = 0.0f;
    };

    CubemapFace CubemapFaceFromIndex(uint32_t faceIndex);
    uint32_t CubemapFaceToIndex(CubemapFace face);

    glm::vec3 DirectionFromFaceTexel(CubemapFace face, uint32_t texelX, uint32_t texelY, uint32_t resolution);
    CubemapFaceSample FaceSampleFromDirection(const glm::vec3 &direction);
}
