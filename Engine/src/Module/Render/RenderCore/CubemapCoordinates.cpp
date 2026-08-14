#include "Hepch.h"
#include "Module/Render/RenderCore/CubemapCoordinates.h"

#include "glm/glm.hpp"

#include <algorithm>

namespace Himii
{
    CubemapFace CubemapFaceFromIndex(uint32_t faceIndex)
    {
        const uint32_t clamped = std::min(faceIndex, CubemapFaceCount - 1u);
        return static_cast<CubemapFace>(clamped);
    }

    uint32_t CubemapFaceToIndex(CubemapFace face)
    {
        return static_cast<uint32_t>(face);
    }

    glm::vec3 DirectionFromFaceTexel(CubemapFace face, uint32_t texelX, uint32_t texelY, uint32_t resolution)
    {
        const float safeResolution = static_cast<float>(std::max(1u, resolution));
        const float signedU = 2.0f * ((static_cast<float>(texelX) + 0.5f) / safeResolution) - 1.0f;
        const float signedV = 2.0f * ((static_cast<float>(texelY) + 0.5f) / safeResolution) - 1.0f;

        switch (face)
        {
            case CubemapFace::PositiveX:
                return glm::normalize(glm::vec3(1.0f, -signedV, -signedU));
            case CubemapFace::NegativeX:
                return glm::normalize(glm::vec3(-1.0f, -signedV, signedU));
            case CubemapFace::PositiveY:
                return glm::normalize(glm::vec3(signedU, 1.0f, signedV));
            case CubemapFace::NegativeY:
                return glm::normalize(glm::vec3(signedU, -1.0f, -signedV));
            case CubemapFace::PositiveZ:
                return glm::normalize(glm::vec3(signedU, -signedV, 1.0f));
            case CubemapFace::NegativeZ:
            default:
                return glm::normalize(glm::vec3(-signedU, -signedV, -1.0f));
        }
    }

    CubemapFaceSample FaceSampleFromDirection(const glm::vec3 &direction)
    {
        const glm::vec3 normalized = glm::normalize(direction);
        const glm::vec3 absolute = glm::abs(normalized);

        CubemapFaceSample sample;
        float signedU = 0.0f;
        float signedV = 0.0f;

        if (absolute.x >= absolute.y && absolute.x >= absolute.z)
        {
            if (normalized.x >= 0.0f)
            {
                sample.Face = CubemapFace::PositiveX;
                const float majorAxis = std::max(normalized.x, 0.0001f);
                signedU = -normalized.z / majorAxis;
                signedV = -normalized.y / majorAxis;
            }
            else
            {
                sample.Face = CubemapFace::NegativeX;
                const float majorAxis = std::max(-normalized.x, 0.0001f);
                signedU = normalized.z / majorAxis;
                signedV = -normalized.y / majorAxis;
            }
        }
        else if (absolute.y >= absolute.x && absolute.y >= absolute.z)
        {
            if (normalized.y >= 0.0f)
            {
                sample.Face = CubemapFace::PositiveY;
                const float majorAxis = std::max(normalized.y, 0.0001f);
                signedU = normalized.x / majorAxis;
                signedV = normalized.z / majorAxis;
            }
            else
            {
                sample.Face = CubemapFace::NegativeY;
                const float majorAxis = std::max(-normalized.y, 0.0001f);
                signedU = normalized.x / majorAxis;
                signedV = -normalized.z / majorAxis;
            }
        }
        else if (normalized.z >= 0.0f)
        {
            sample.Face = CubemapFace::PositiveZ;
            const float majorAxis = std::max(normalized.z, 0.0001f);
            signedU = normalized.x / majorAxis;
            signedV = -normalized.y / majorAxis;
        }
        else
        {
            sample.Face = CubemapFace::NegativeZ;
            const float majorAxis = std::max(-normalized.z, 0.0001f);
            signedU = -normalized.x / majorAxis;
            signedV = -normalized.y / majorAxis;
        }

        sample.ImageU = glm::clamp(0.5f * (signedU + 1.0f), 0.0f, 1.0f);
        sample.ImageV = glm::clamp(0.5f * (signedV + 1.0f), 0.0f, 1.0f);
        return sample;
    }
}
