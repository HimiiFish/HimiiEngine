#include "Hepch.h"
#include "Module/Render/Environment/EnvironmentLightingSystem.h"
#include "Module/Render/Environment/EnvironmentMapAsset.h"

#include "EngineCore/Core/FileSystem.h"
#include "EngineCore/Core/Log.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "stb_image.h"

namespace Himii
{
    namespace
    {
        constexpr float Pi = 3.14159265358979323846f;
        constexpr char BakeCacheMagic[8] = {'H', 'I', 'M', 'I', 'I', 'E', 'N', 'V'};
        constexpr uint32_t BakeCacheVersion = 1;

        struct EquirectangularImage
        {
            int Width = 0;
            int Height = 0;
            std::vector<float> Rgb;
        };

        struct RuntimeBakeCacheEntry
        {
            BakedEnvironmentLighting Lighting;
            std::string SourceContentHash;
            std::filesystem::path SourceFilesystemPath;
            uint64_t MetaWriteTimeCount = 0;
        };

        std::mutex s_SystemMutex;
        bool s_Initialized = false;
        Ref<Texture2D> s_SharedBrdfLookup;
        std::unordered_map<uint64_t, RuntimeBakeCacheEntry> s_RuntimeCache;

        glm::vec3 FaceDirection(uint32_t faceIndex, float uniqueCoordinateX, float uniqueCoordinateY)
        {
            switch (faceIndex)
            {
                case 0:
                    return glm::normalize(glm::vec3(1.0f, uniqueCoordinateY, -uniqueCoordinateX));
                case 1:
                    return glm::normalize(glm::vec3(-1.0f, uniqueCoordinateY, uniqueCoordinateX));
                case 2:
                    return glm::normalize(glm::vec3(uniqueCoordinateX, 1.0f, -uniqueCoordinateY));
                case 3:
                    return glm::normalize(glm::vec3(uniqueCoordinateX, -1.0f, uniqueCoordinateY));
                case 4:
                    return glm::normalize(glm::vec3(uniqueCoordinateX, uniqueCoordinateY, 1.0f));
                default:
                    return glm::normalize(glm::vec3(-uniqueCoordinateX, uniqueCoordinateY, -1.0f));
            }
        }

        glm::vec3 SampleEquirectangular(const EquirectangularImage &image, const glm::vec3 &direction)
        {
            const glm::vec3 normalized = glm::normalize(direction);
            const float longitude = std::atan2(normalized.z, normalized.x);
            const float latitude = std::asin(glm::clamp(normalized.y, -1.0f, 1.0f));
            float uniqueCoordinateX = (longitude + Pi) / (2.0f * Pi);
            float uniqueCoordinateY = 0.5f - latitude / Pi;
            uniqueCoordinateX = glm::clamp(uniqueCoordinateX, 0.0f, 1.0f);
            uniqueCoordinateY = glm::clamp(uniqueCoordinateY, 0.0f, 1.0f);

            const float sampleX = uniqueCoordinateX * static_cast<float>(image.Width - 1);
            const float sampleY = uniqueCoordinateY * static_cast<float>(image.Height - 1);
            const int x0 = static_cast<int>(sampleX);
            const int y0 = static_cast<int>(sampleY);
            const int x1 = std::min(x0 + 1, image.Width - 1);
            const int y1 = std::min(y0 + 1, image.Height - 1);
            const float fractionX = sampleX - static_cast<float>(x0);
            const float fractionY = sampleY - static_cast<float>(y0);

            auto fetch = [&](int x, int y) {
                const size_t index = (static_cast<size_t>(y) * static_cast<size_t>(image.Width)
                                     + static_cast<size_t>(x))
                                    * 3u;
                return glm::vec3(image.Rgb[index], image.Rgb[index + 1], image.Rgb[index + 2]);
            };

            const glm::vec3 color00 = fetch(x0, y0);
            const glm::vec3 color10 = fetch(x1, y0);
            const glm::vec3 color01 = fetch(x0, y1);
            const glm::vec3 color11 = fetch(x1, y1);
            const glm::vec3 color0 = glm::mix(color00, color10, fractionX);
            const glm::vec3 color1 = glm::mix(color01, color11, fractionX);
            return glm::mix(color0, color1, fractionY);
        }

        bool LoadEquirectangularHdr(const std::filesystem::path &path, EquirectangularImage &outImage)
        {
            const auto fileBytes = FileSystem::ReadBytes(path.string());
            if (!fileBytes || fileBytes->empty())
            {
                HIMII_CORE_ERROR("Failed to read HDR environment: {0}", path.string());
                return false;
            }

            int width = 0;
            int height = 0;
            int channels = 0;
            stbi_set_flip_vertically_on_load(false);
            float *data = stbi_loadf_from_memory(fileBytes->data(), static_cast<int>(fileBytes->size()), &width,
                                                 &height, &channels, 3);
            if (!data)
            {
                HIMII_CORE_ERROR("stbi_loadf failed for HDR: {0}", path.string());
                return false;
            }

            outImage.Width = width;
            outImage.Height = height;
            outImage.Rgb.assign(data, data + static_cast<size_t>(width) * static_cast<size_t>(height) * 3u);
            stbi_image_free(data);
            return true;
        }

        void ConvertEquirectangularToCubemap(const EquirectangularImage &image, uint32_t resolution,
                                             std::vector<float> &outFacesRgb)
        {
            outFacesRgb.assign(static_cast<size_t>(6) * resolution * resolution * 3u, 0.0f);
            for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                for (uint32_t y = 0; y < resolution; ++y)
                {
                    for (uint32_t x = 0; x < resolution; ++x)
                    {
                        const float uniqueCoordinateX =
                                2.0f * ((static_cast<float>(x) + 0.5f) / static_cast<float>(resolution)) - 1.0f;
                        const float uniqueCoordinateY =
                                2.0f * ((static_cast<float>(y) + 0.5f) / static_cast<float>(resolution)) - 1.0f;
                        const glm::vec3 direction = FaceDirection(faceIndex, uniqueCoordinateX, -uniqueCoordinateY);
                        const glm::vec3 color = SampleEquirectangular(image, direction);
                        const size_t pixelIndex =
                                (static_cast<size_t>(faceIndex) * resolution * resolution
                                 + static_cast<size_t>(y) * resolution + x)
                                * 3u;
                        outFacesRgb[pixelIndex] = color.r;
                        outFacesRgb[pixelIndex + 1] = color.g;
                        outFacesRgb[pixelIndex + 2] = color.b;
                    }
                }
            }
        }

        glm::vec3 SampleCubemapCpu(const std::vector<float> &facesRgb, uint32_t resolution,
                                   const glm::vec3 &direction)
        {
            const glm::vec3 absolute = glm::abs(direction);
            uint32_t faceIndex = 0;
            float maxAxis = absolute.x;
            float uniqueCoordinateX = -direction.z;
            float uniqueCoordinateY = direction.y;
            if (absolute.y >= maxAxis)
            {
                maxAxis = absolute.y;
                faceIndex = direction.y > 0.0f ? 2u : 3u;
                uniqueCoordinateX = direction.x;
                uniqueCoordinateY = direction.y > 0.0f ? -direction.z : direction.z;
            }
            if (absolute.z >= maxAxis)
            {
                faceIndex = direction.z > 0.0f ? 4u : 5u;
                uniqueCoordinateX = direction.z > 0.0f ? direction.x : -direction.x;
                uniqueCoordinateY = direction.y;
                maxAxis = absolute.z;
            }
            else if (absolute.x >= absolute.y)
            {
                faceIndex = direction.x > 0.0f ? 0u : 1u;
                uniqueCoordinateX = direction.x > 0.0f ? -direction.z : direction.z;
                uniqueCoordinateY = direction.y;
                maxAxis = absolute.x;
            }

            const float safeMax = std::max(maxAxis, 0.0001f);
            const float normalizedX = 0.5f * (uniqueCoordinateX / safeMax + 1.0f);
            const float normalizedY = 0.5f * (uniqueCoordinateY / safeMax + 1.0f);
            const int x = glm::clamp(static_cast<int>(normalizedX * static_cast<float>(resolution - 1)), 0,
                                     static_cast<int>(resolution - 1));
            const int y = glm::clamp(static_cast<int>(normalizedY * static_cast<float>(resolution - 1)), 0,
                                     static_cast<int>(resolution - 1));
            const size_t pixelIndex =
                    (static_cast<size_t>(faceIndex) * resolution * resolution
                     + static_cast<size_t>(y) * resolution + static_cast<size_t>(x))
                    * 3u;
            return glm::vec3(facesRgb[pixelIndex], facesRgb[pixelIndex + 1], facesRgb[pixelIndex + 2]);
        }

        void ConvolveIrradiance(const std::vector<float> &environmentFaces, uint32_t environmentResolution,
                                uint32_t irradianceResolution, std::vector<float> &outIrradianceFaces)
        {
            outIrradianceFaces.assign(static_cast<size_t>(6) * irradianceResolution * irradianceResolution * 3u,
                                      0.0f);
            constexpr uint32_t sampleCountPhi = 32;
            constexpr uint32_t sampleCountTheta = 16;

            for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                for (uint32_t y = 0; y < irradianceResolution; ++y)
                {
                    for (uint32_t x = 0; x < irradianceResolution; ++x)
                    {
                        const float uniqueCoordinateX =
                                2.0f * ((static_cast<float>(x) + 0.5f) / static_cast<float>(irradianceResolution))
                                - 1.0f;
                        const float uniqueCoordinateY =
                                2.0f * ((static_cast<float>(y) + 0.5f) / static_cast<float>(irradianceResolution))
                                - 1.0f;
                        const glm::vec3 normal = FaceDirection(faceIndex, uniqueCoordinateX, -uniqueCoordinateY);
                        glm::vec3 up = std::abs(normal.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                                   : glm::vec3(0.0f, 0.0f, 1.0f);
                        const glm::vec3 right = glm::normalize(glm::cross(up, normal));
                        up = glm::normalize(glm::cross(normal, right));

                        glm::vec3 irradiance{0.0f};
                        float sampleWeight = 0.0f;
                        for (uint32_t phiIndex = 0; phiIndex < sampleCountPhi; ++phiIndex)
                        {
                            const float phi = 2.0f * Pi * (static_cast<float>(phiIndex) + 0.5f)
                                              / static_cast<float>(sampleCountPhi);
                            for (uint32_t thetaIndex = 0; thetaIndex < sampleCountTheta; ++thetaIndex)
                            {
                                const float theta = 0.5f * Pi * (static_cast<float>(thetaIndex) + 0.5f)
                                                    / static_cast<float>(sampleCountTheta);
                                const glm::vec3 tangentSample(std::sin(theta) * std::cos(phi),
                                                              std::sin(theta) * std::sin(phi), std::cos(theta));
                                const glm::vec3 sampleDirection =
                                        tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
                                const glm::vec3 color =
                                        SampleCubemapCpu(environmentFaces, environmentResolution, sampleDirection);
                                irradiance += color * std::cos(theta) * std::sin(theta);
                                sampleWeight += std::sin(theta);
                            }
                        }

                        irradiance = Pi * irradiance / std::max(sampleWeight, 0.0001f);
                        const size_t pixelIndex =
                                (static_cast<size_t>(faceIndex) * irradianceResolution * irradianceResolution
                                 + static_cast<size_t>(y) * irradianceResolution + x)
                                * 3u;
                        outIrradianceFaces[pixelIndex] = irradiance.r;
                        outIrradianceFaces[pixelIndex + 1] = irradiance.g;
                        outIrradianceFaces[pixelIndex + 2] = irradiance.b;
                    }
                }
            }
        }

        float RadicalInverseVanDerCorput(uint32_t bits)
        {
            bits = (bits << 16u) | (bits >> 16u);
            bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
            bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
            bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
            bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
            return static_cast<float>(bits) * 2.3283064365386963e-10f;
        }

        glm::vec2 Hammersley(uint32_t index, uint32_t total)
        {
            return {static_cast<float>(index) / static_cast<float>(total), RadicalInverseVanDerCorput(index)};
        }

        glm::vec3 ImportanceSampleGGX(glm::vec2 xi, glm::vec3 normal, float roughness)
        {
            const float alpha = roughness * roughness;
            const float phi = 2.0f * Pi * xi.x;
            const float cosineTheta =
                    std::sqrt((1.0f - xi.y) / (1.0f + (alpha * alpha - 1.0f) * xi.y));
            const float sineTheta = std::sqrt(std::max(0.0f, 1.0f - cosineTheta * cosineTheta));

            glm::vec3 halfway(std::cos(phi) * sineTheta, std::sin(phi) * sineTheta, cosineTheta);
            const glm::vec3 up = std::abs(normal.z) < 0.999f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                                            : glm::vec3(1.0f, 0.0f, 0.0f);
            const glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
            const glm::vec3 bitangent = glm::cross(normal, tangent);
            return glm::normalize(tangent * halfway.x + bitangent * halfway.y + normal * halfway.z);
        }

        void ConvolvePrefilter(const std::vector<float> &environmentFaces, uint32_t environmentResolution,
                               uint32_t prefilterResolution, uint32_t mipCount,
                               std::vector<std::vector<float>> &outMipFaces)
        {
            outMipFaces.resize(mipCount);
            constexpr uint32_t sampleCount = 64;

            for (uint32_t mipLevel = 0; mipLevel < mipCount; ++mipLevel)
            {
                const uint32_t mipSize = std::max(1u, prefilterResolution >> mipLevel);
                const float roughness =
                        mipCount <= 1 ? 0.0f
                                      : static_cast<float>(mipLevel) / static_cast<float>(mipCount - 1);
                outMipFaces[mipLevel].assign(static_cast<size_t>(6) * mipSize * mipSize * 3u, 0.0f);

                for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
                {
                    for (uint32_t y = 0; y < mipSize; ++y)
                    {
                        for (uint32_t x = 0; x < mipSize; ++x)
                        {
                            const float uniqueCoordinateX =
                                    2.0f * ((static_cast<float>(x) + 0.5f) / static_cast<float>(mipSize)) - 1.0f;
                            const float uniqueCoordinateY =
                                    2.0f * ((static_cast<float>(y) + 0.5f) / static_cast<float>(mipSize)) - 1.0f;
                            const glm::vec3 normal =
                                    FaceDirection(faceIndex, uniqueCoordinateX, -uniqueCoordinateY);
                            const glm::vec3 viewDirection = normal;

                            glm::vec3 prefilteredColor{0.0f};
                            float totalWeight = 0.0f;
                            for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
                            {
                                const glm::vec2 xi = Hammersley(sampleIndex, sampleCount);
                                const glm::vec3 halfway = ImportanceSampleGGX(xi, normal, roughness);
                                const glm::vec3 lightDirection =
                                        glm::normalize(2.0f * glm::dot(viewDirection, halfway) * halfway
                                                       - viewDirection);
                                const float normalDotLight = glm::max(glm::dot(normal, lightDirection), 0.0f);
                                if (normalDotLight > 0.0f)
                                {
                                    prefilteredColor += SampleCubemapCpu(environmentFaces, environmentResolution,
                                                                         lightDirection)
                                                        * normalDotLight;
                                    totalWeight += normalDotLight;
                                }
                            }

                            prefilteredColor /= std::max(totalWeight, 0.0001f);
                            const size_t pixelIndex =
                                    (static_cast<size_t>(faceIndex) * mipSize * mipSize
                                     + static_cast<size_t>(y) * mipSize + x)
                                    * 3u;
                            outMipFaces[mipLevel][pixelIndex] = prefilteredColor.r;
                            outMipFaces[mipLevel][pixelIndex + 1] = prefilteredColor.g;
                            outMipFaces[mipLevel][pixelIndex + 2] = prefilteredColor.b;
                        }
                    }
                }
            }
        }

        glm::vec2 IntegrateBrdf(float normalDotView, float roughness)
        {
            glm::vec3 viewDirection(std::sqrt(std::max(0.0f, 1.0f - normalDotView * normalDotView)), 0.0f,
                                   normalDotView);
            float scale = 0.0f;
            float bias = 0.0f;
            constexpr uint32_t sampleCount = 128;
            const glm::vec3 normal(0.0f, 0.0f, 1.0f);

            for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
            {
                const glm::vec2 xi = Hammersley(sampleIndex, sampleCount);
                const glm::vec3 halfway = ImportanceSampleGGX(xi, normal, roughness);
                const glm::vec3 lightDirection =
                        glm::normalize(2.0f * glm::dot(viewDirection, halfway) * halfway - viewDirection);

                const float normalDotLight = std::max(lightDirection.z, 0.0f);
                const float normalDotHalfway = std::max(halfway.z, 0.0f);
                const float viewDotHalfway = std::max(glm::dot(viewDirection, halfway), 0.0f);

                if (normalDotLight > 0.0f)
                {
                    const float geometrySmith =
                            (normalDotView / (normalDotView * (1.0f - roughness / 2.0f) + roughness / 2.0f))
                            * (normalDotLight
                               / (normalDotLight * (1.0f - roughness / 2.0f) + roughness / 2.0f));
                    const float geometryVisibility =
                            (geometrySmith * viewDotHalfway) / std::max(normalDotHalfway * normalDotView, 0.0001f);
                    const float fresnelFactor = std::pow(1.0f - viewDotHalfway, 5.0f);
                    scale += (1.0f - fresnelFactor) * geometryVisibility;
                    bias += fresnelFactor * geometryVisibility;
                }
            }

            scale /= static_cast<float>(sampleCount);
            bias /= static_cast<float>(sampleCount);
            return {scale, bias};
        }

        Ref<Texture2D> GenerateBrdfLookupTexture(uint32_t size)
        {
            TextureSpecification specification;
            specification.Width = size;
            specification.Height = size;
            specification.Format = ImageFormat::RG16F;
            specification.ClampToEdge = true;
            specification.UseLinearFiltering = true;
            Ref<Texture2D> texture = Texture2D::Create(specification);

            std::vector<float> pixels(static_cast<size_t>(size) * size * 2u, 0.0f);
            for (uint32_t y = 0; y < size; ++y)
            {
                for (uint32_t x = 0; x < size; ++x)
                {
                    const float normalDotView = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
                    const float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
                    const glm::vec2 integrated = IntegrateBrdf(std::max(normalDotView, 0.001f), roughness);
                    const size_t index = (static_cast<size_t>(y) * size + x) * 2u;
                    pixels[index] = integrated.x;
                    pixels[index + 1] = integrated.y;
                }
            }

            texture->SetData(pixels.data(), static_cast<uint32_t>(pixels.size() * sizeof(float)));
            return texture;
        }

        Ref<TextureCube> UploadCubemapFaces(const std::vector<float> &facesRgb, uint32_t resolution,
                                            bool generateMips)
        {
            TextureSpecification specification;
            specification.Width = resolution;
            specification.Height = resolution;
            specification.Format = ImageFormat::RGB16F;
            specification.GenerateMips = generateMips;
            specification.ClampToEdge = true;
            specification.UseLinearFiltering = true;
            Ref<TextureCube> cubemap = TextureCube::Create(specification);
            const uint32_t faceByteCount = resolution * resolution * 3u * static_cast<uint32_t>(sizeof(float));
            for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                const float *faceData =
                        facesRgb.data()
                        + static_cast<size_t>(faceIndex) * resolution * resolution * 3u;
                cubemap->SetFaceData(faceIndex, 0, faceData, faceByteCount);
            }
            return cubemap;
        }

        Ref<TextureCube> UploadPrefilteredCubemap(const std::vector<std::vector<float>> &mipFaces,
                                                  uint32_t baseResolution)
        {
            TextureSpecification specification;
            specification.Width = baseResolution;
            specification.Height = baseResolution;
            specification.Format = ImageFormat::RGB16F;
            specification.GenerateMips = true;
            specification.ClampToEdge = true;
            specification.UseLinearFiltering = true;
            Ref<TextureCube> cubemap = TextureCube::Create(specification);

            for (uint32_t mipLevel = 0; mipLevel < mipFaces.size(); ++mipLevel)
            {
                const uint32_t mipSize = std::max(1u, baseResolution >> mipLevel);
                const uint32_t faceByteCount = mipSize * mipSize * 3u * static_cast<uint32_t>(sizeof(float));
                for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
                {
                    const float *faceData =
                            mipFaces[mipLevel].data()
                            + static_cast<size_t>(faceIndex) * mipSize * mipSize * 3u;
                    cubemap->SetFaceData(faceIndex, mipLevel, faceData, faceByteCount);
                }
            }
            return cubemap;
        }

        std::filesystem::path MakeBakeCacheFilePath(AssetHandle handle, const std::string &sourceHash)
        {
            std::ostringstream nameStream;
            nameStream << std::hex << static_cast<uint64_t>(handle) << "_" << sourceHash << ".ienv";
            return Project::GetEnvironmentBakeCacheDirectory() / nameStream.str();
        }

        bool WriteBakeCacheFile(const std::filesystem::path &path, uint32_t cubemapResolution,
                                const std::vector<float> &environmentFaces, uint32_t irradianceResolution,
                                const std::vector<float> &irradianceFaces, uint32_t prefilterResolution,
                                uint32_t prefilterMipCount, const std::vector<std::vector<float>> &prefilterMips)
        {
            std::error_code createError;
            std::filesystem::create_directories(path.parent_path(), createError);
            std::ofstream file(path, std::ios::binary);
            if (!file.is_open())
                return false;

            file.write(BakeCacheMagic, sizeof(BakeCacheMagic));
            file.write(reinterpret_cast<const char *>(&BakeCacheVersion), sizeof(BakeCacheVersion));
            file.write(reinterpret_cast<const char *>(&cubemapResolution), sizeof(cubemapResolution));
            file.write(reinterpret_cast<const char *>(environmentFaces.data()),
                       static_cast<std::streamsize>(environmentFaces.size() * sizeof(float)));
            file.write(reinterpret_cast<const char *>(&irradianceResolution), sizeof(irradianceResolution));
            file.write(reinterpret_cast<const char *>(irradianceFaces.data()),
                       static_cast<std::streamsize>(irradianceFaces.size() * sizeof(float)));
            file.write(reinterpret_cast<const char *>(&prefilterResolution), sizeof(prefilterResolution));
            file.write(reinterpret_cast<const char *>(&prefilterMipCount), sizeof(prefilterMipCount));
            for (uint32_t mipLevel = 0; mipLevel < prefilterMipCount; ++mipLevel)
            {
                file.write(reinterpret_cast<const char *>(prefilterMips[mipLevel].data()),
                           static_cast<std::streamsize>(prefilterMips[mipLevel].size() * sizeof(float)));
            }
            return static_cast<bool>(file);
        }

        bool ReadBakeCacheFile(const std::filesystem::path &path, uint32_t expectedCubemapResolution,
                               uint32_t expectedIrradianceResolution, uint32_t expectedPrefilterResolution,
                               uint32_t expectedPrefilterMipCount, std::vector<float> &environmentFaces,
                               std::vector<float> &irradianceFaces,
                               std::vector<std::vector<float>> &prefilterMips)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open())
                return false;

            char magic[sizeof(BakeCacheMagic)] = {};
            file.read(magic, sizeof(BakeCacheMagic));
            if (!file || std::memcmp(magic, BakeCacheMagic, sizeof(BakeCacheMagic)) != 0)
                return false;

            uint32_t version = 0;
            file.read(reinterpret_cast<char *>(&version), sizeof(version));
            if (version != BakeCacheVersion)
                return false;

            uint32_t cubemapResolution = 0;
            file.read(reinterpret_cast<char *>(&cubemapResolution), sizeof(cubemapResolution));
            if (cubemapResolution != expectedCubemapResolution)
                return false;

            environmentFaces.resize(static_cast<size_t>(6) * cubemapResolution * cubemapResolution * 3u);
            file.read(reinterpret_cast<char *>(environmentFaces.data()),
                      static_cast<std::streamsize>(environmentFaces.size() * sizeof(float)));

            uint32_t irradianceResolution = 0;
            file.read(reinterpret_cast<char *>(&irradianceResolution), sizeof(irradianceResolution));
            if (irradianceResolution != expectedIrradianceResolution)
                return false;
            irradianceFaces.resize(static_cast<size_t>(6) * irradianceResolution * irradianceResolution * 3u);
            file.read(reinterpret_cast<char *>(irradianceFaces.data()),
                      static_cast<std::streamsize>(irradianceFaces.size() * sizeof(float)));

            uint32_t prefilterResolution = 0;
            uint32_t prefilterMipCount = 0;
            file.read(reinterpret_cast<char *>(&prefilterResolution), sizeof(prefilterResolution));
            file.read(reinterpret_cast<char *>(&prefilterMipCount), sizeof(prefilterMipCount));
            if (prefilterResolution != expectedPrefilterResolution
                || prefilterMipCount != expectedPrefilterMipCount)
                return false;

            prefilterMips.resize(prefilterMipCount);
            for (uint32_t mipLevel = 0; mipLevel < prefilterMipCount; ++mipLevel)
            {
                const uint32_t mipSize = std::max(1u, prefilterResolution >> mipLevel);
                prefilterMips[mipLevel].resize(static_cast<size_t>(6) * mipSize * mipSize * 3u);
                file.read(reinterpret_cast<char *>(prefilterMips[mipLevel].data()),
                          static_cast<std::streamsize>(prefilterMips[mipLevel].size() * sizeof(float)));
            }

            return static_cast<bool>(file);
        }

        uint64_t QueryMetaWriteTimeCount(const std::filesystem::path &metaPath)
        {
            std::error_code errorCode;
            const auto writeTime = std::filesystem::last_write_time(metaPath, errorCode);
            if (errorCode)
                return 0;
            return static_cast<uint64_t>(writeTime.time_since_epoch().count());
        }
    }

    void EnvironmentLightingSystem::Init()
    {
        std::scoped_lock lock(s_SystemMutex);
        if (s_Initialized)
            return;
        s_SharedBrdfLookup = GenerateBrdfLookupTexture(256);
        s_Initialized = true;
    }

    void EnvironmentLightingSystem::Shutdown()
    {
        std::scoped_lock lock(s_SystemMutex);
        s_RuntimeCache.clear();
        s_SharedBrdfLookup.reset();
        s_Initialized = false;
    }

    Ref<Texture2D> EnvironmentLightingSystem::GetSharedBrdfLookupTexture()
    {
        std::scoped_lock lock(s_SystemMutex);
        if (!s_Initialized)
            s_SharedBrdfLookup = GenerateBrdfLookupTexture(256);
        return s_SharedBrdfLookup;
    }

    void EnvironmentLightingSystem::Invalidate(AssetHandle environmentMapHandle)
    {
        std::scoped_lock lock(s_SystemMutex);
        s_RuntimeCache.erase(static_cast<uint64_t>(environmentMapHandle));
    }

    void EnvironmentLightingSystem::PollSourceChanges()
    {
        std::scoped_lock lock(s_SystemMutex);
        std::vector<uint64_t> staleHandles;
        for (const auto &[handleValue, entry] : s_RuntimeCache)
        {
            const std::string currentHash =
                    EnvironmentMapImportSerializer::ComputeSourceContentHash(entry.SourceFilesystemPath);
            const uint64_t metaWriteTime = QueryMetaWriteTimeCount(
                    EnvironmentMapImportSerializer::GetMetaPath(entry.SourceFilesystemPath));
            if (currentHash != entry.SourceContentHash || metaWriteTime != entry.MetaWriteTimeCount)
                staleHandles.push_back(handleValue);
        }
        for (uint64_t handleValue : staleHandles)
            s_RuntimeCache.erase(handleValue);
    }

    BakedEnvironmentLighting EnvironmentLightingSystem::EnsureBaked(AssetHandle environmentMapHandle)
    {
        BakedEnvironmentLighting result{};
        if (environmentMapHandle == 0 || !Project::GetActive() || !ResourceSystem::IsBound())
            return result;

        Init();

        Ref<AssetManager> assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager || !assetManager->IsAssetHandleValid(environmentMapHandle))
            return result;

        Ref<Asset> assetBase = assetManager->GetAsset(environmentMapHandle);
        if (!assetBase || assetBase->GetType() != AssetType::EnvironmentMap)
            return result;

        Ref<EnvironmentMapAsset> environmentAsset = std::static_pointer_cast<EnvironmentMapAsset>(assetBase);
        std::filesystem::path sourcePath = environmentAsset->SourceFilePath;
        if (!sourcePath.is_absolute())
            sourcePath = Project::GetAssetFileSystemPath(sourcePath);
        EnvironmentMapImportSerializer::EnsureDefaultMeta(sourcePath);

        EnvironmentImportSettings settings = environmentAsset->ImportSettings;
        EnvironmentMapImportSerializer::Deserialize(sourcePath, settings);
        environmentAsset->ImportSettings = settings;

        const uint64_t handleValue = static_cast<uint64_t>(environmentMapHandle);
        const uint64_t metaWriteTime =
                QueryMetaWriteTimeCount(EnvironmentMapImportSerializer::GetMetaPath(sourcePath));

        {
            std::scoped_lock lock(s_SystemMutex);
            const auto cacheIterator = s_RuntimeCache.find(handleValue);
            if (cacheIterator != s_RuntimeCache.end()
                && cacheIterator->second.SourceContentHash == settings.SourceContentHash
                && cacheIterator->second.MetaWriteTimeCount == metaWriteTime
                && cacheIterator->second.Lighting.Valid)
            {
                return cacheIterator->second.Lighting;
            }
        }

        std::vector<float> environmentFaces;
        std::vector<float> irradianceFaces;
        std::vector<std::vector<float>> prefilterMips;
        const std::filesystem::path cachePath =
                MakeBakeCacheFilePath(environmentMapHandle, settings.SourceContentHash);

        const bool loadedFromDisk = ReadBakeCacheFile(cachePath, settings.CubemapResolution,
                                                      settings.IrradianceResolution, settings.PrefilterResolution,
                                                      settings.PrefilterMipCount, environmentFaces, irradianceFaces,
                                                      prefilterMips);
        if (!loadedFromDisk)
        {
            HIMII_CORE_INFO("Baking environment IBL for {0} ...", sourcePath.string());
            EquirectangularImage equirectangular;
            if (!LoadEquirectangularHdr(sourcePath, equirectangular))
                return result;

            ConvertEquirectangularToCubemap(equirectangular, settings.CubemapResolution, environmentFaces);
            ConvolveIrradiance(environmentFaces, settings.CubemapResolution, settings.IrradianceResolution,
                               irradianceFaces);
            ConvolvePrefilter(environmentFaces, settings.CubemapResolution, settings.PrefilterResolution,
                              settings.PrefilterMipCount, prefilterMips);
            if (!WriteBakeCacheFile(cachePath, settings.CubemapResolution, environmentFaces,
                                    settings.IrradianceResolution, irradianceFaces, settings.PrefilterResolution,
                                    settings.PrefilterMipCount, prefilterMips))
            {
                HIMII_CORE_WARNING("Failed to persist environment bake cache: {0}", cachePath.string());
            }
            else
            {
                HIMII_CORE_INFO("Environment bake cache written: {0}", cachePath.string());
            }
        }

        result.EnvironmentCubemap = UploadCubemapFaces(environmentFaces, settings.CubemapResolution, false);
        result.IrradianceCubemap = UploadCubemapFaces(irradianceFaces, settings.IrradianceResolution, false);
        result.PrefilteredCubemap = UploadPrefilteredCubemap(prefilterMips, settings.PrefilterResolution);
        result.BrdfLookupTexture = GetSharedBrdfLookupTexture();
        result.Valid = result.EnvironmentCubemap && result.IrradianceCubemap && result.PrefilteredCubemap
                       && result.BrdfLookupTexture;

        if (result.Valid)
        {
            std::scoped_lock lock(s_SystemMutex);
            RuntimeBakeCacheEntry entry;
            entry.Lighting = result;
            entry.SourceContentHash = settings.SourceContentHash;
            entry.SourceFilesystemPath = sourcePath;
            entry.MetaWriteTimeCount = metaWriteTime;
            s_RuntimeCache[handleValue] = std::move(entry);
        }

        return result;
    }
}
