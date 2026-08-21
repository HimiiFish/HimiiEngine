#include "panel/MaterialThumbnailUtility.h"

#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/MeshTangentUtility.h"
#include "Module/Render/RenderCore/Framebuffer.h"
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/Renderer/Renderer3D.h"
#include "Module/Render/RenderCore/Camera.h"
#include "Resource/AssetManager.h"

#include <cmath>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    namespace
    {
        constexpr uint32_t MaterialThumbnailPixelSize = 128;
        constexpr float MaterialThumbnailBackgroundRed = 42.0f / 255.0f;
        constexpr float MaterialThumbnailBackgroundGreen = 42.0f / 255.0f;
        constexpr float MaterialThumbnailBackgroundBlue = 42.0f / 255.0f;

        std::unordered_map<uint64_t, Ref<Texture2D>> s_MaterialThumbnailCache;
        std::deque<AssetHandle> s_PendingMaterialThumbnailQueue;
        std::unordered_set<uint64_t> s_PendingMaterialThumbnailKeys;

        Ref<Framebuffer> s_MaterialThumbnailFramebuffer;
        Ref<MeshAsset> s_MaterialThumbnailSphereMesh;

        Ref<MeshAsset> BuildUnitSphereMeshAsset()
        {
            Ref<MeshAsset> meshAsset = CreateRef<MeshAsset>();
            const int stackCount = 24;
            const int sectorCount = 48;
            const float radius = 0.5f;

            for (int stackIndex = 0; stackIndex <= stackCount; ++stackIndex)
            {
                const float stackAngle =
                        glm::pi<float>() / 2.0f
                        - static_cast<float>(stackIndex) * glm::pi<float>() / static_cast<float>(stackCount);
                const float ringRadius = radius * std::cos(stackAngle);
                const float positionY = radius * std::sin(stackAngle);

                for (int sectorIndex = 0; sectorIndex <= sectorCount; ++sectorIndex)
                {
                    const float sectorAngle =
                            static_cast<float>(sectorIndex) * 2.0f * glm::pi<float>()
                            / static_cast<float>(sectorCount);
                    MeshVertex vertex;
                    vertex.Position = {ringRadius * std::cos(sectorAngle), positionY,
                                       ringRadius * std::sin(sectorAngle)};
                    vertex.Normal = glm::normalize(vertex.Position);
                    vertex.TextureCoordinate = {static_cast<float>(sectorIndex) / static_cast<float>(sectorCount),
                                               static_cast<float>(stackIndex) / static_cast<float>(stackCount)};
                    meshAsset->Vertices.push_back(vertex);
                }
            }

            for (int stackIndex = 0; stackIndex < stackCount; ++stackIndex)
            {
                int firstRing = stackIndex * (sectorCount + 1);
                int secondRing = firstRing + sectorCount + 1;
                for (int sectorIndex = 0; sectorIndex < sectorCount; ++sectorIndex, ++firstRing, ++secondRing)
                {
                    if (stackIndex != 0)
                    {
                        meshAsset->Indices.push_back(static_cast<uint32_t>(firstRing));
                        meshAsset->Indices.push_back(static_cast<uint32_t>(firstRing + 1));
                        meshAsset->Indices.push_back(static_cast<uint32_t>(secondRing));
                    }
                    if (stackIndex != (stackCount - 1))
                    {
                        meshAsset->Indices.push_back(static_cast<uint32_t>(firstRing + 1));
                        meshAsset->Indices.push_back(static_cast<uint32_t>(secondRing + 1));
                        meshAsset->Indices.push_back(static_cast<uint32_t>(secondRing));
                    }
                }
            }

            MeshSubmesh submesh;
            submesh.IndexStart = 0;
            submesh.IndexCount = static_cast<uint32_t>(meshAsset->Indices.size());
            submesh.MaterialSlotIndex = 0;
            meshAsset->Submeshes.push_back(submesh);
            meshAsset->MaterialSlotNames.push_back("Material");
            GenerateMeshTangents(*meshAsset);
            return meshAsset;
        }

        void EnsureMaterialThumbnailResources()
        {
            if (!s_MaterialThumbnailFramebuffer)
            {
                FramebufferSpecification specification;
                specification.Width = MaterialThumbnailPixelSize;
                specification.Height = MaterialThumbnailPixelSize;
                specification.Attachments = {FramebufferFormat::RGBA8, FramebufferFormat::DEPTH24STENCIL8};
                s_MaterialThumbnailFramebuffer = Framebuffer::Create(specification);
            }

            if (!s_MaterialThumbnailSphereMesh)
                s_MaterialThumbnailSphereMesh = BuildUnitSphereMeshAsset();
        }

        SceneLightingParameters BuildStudioLighting()
        {
            SceneLightingParameters lighting;
            lighting.HasDirectionalLight = true;
            lighting.DirectionalLightDirection = glm::normalize(glm::vec3(-0.35f, -0.75f, -0.55f));
            lighting.DirectionalLightColor = glm::vec3(1.0f, 0.98f, 0.95f);
            lighting.DirectionalLightIntensity = 1.35f;
            lighting.AmbientColor = glm::vec3(1.0f, 1.0f, 1.0f);
            lighting.AmbientIntensity = 0.22f;
            lighting.HasShadowMap = false;
            lighting.PointLightCount = 1;
            lighting.PointLights[0].Position = glm::vec3(1.2f, 1.4f, 1.6f);
            lighting.PointLights[0].Color = glm::vec3(1.0f, 0.95f, 0.9f);
            lighting.PointLights[0].Intensity = 0.55f;
            lighting.PointLights[0].Range = 8.0f;
            return lighting;
        }

        void FlipRgbaRowsVertically(std::vector<uint8_t> &rgbaBytes, uint32_t width, uint32_t height)
        {
            const size_t rowByteCount = static_cast<size_t>(width) * 4u;
            std::vector<uint8_t> temporaryRow(rowByteCount);
            for (uint32_t rowIndex = 0; rowIndex < height / 2u; ++rowIndex)
            {
                uint8_t *topRow = rgbaBytes.data() + static_cast<size_t>(rowIndex) * rowByteCount;
                uint8_t *bottomRow =
                        rgbaBytes.data() + static_cast<size_t>(height - 1u - rowIndex) * rowByteCount;
                std::memcpy(temporaryRow.data(), topRow, rowByteCount);
                std::memcpy(topRow, bottomRow, rowByteCount);
                std::memcpy(bottomRow, temporaryRow.data(), rowByteCount);
            }
        }

        Ref<Texture2D> RenderMaterialThumbnail(AssetManager *assetManager, AssetHandle materialHandle)
        {
            EnsureMaterialThumbnailResources();
            if (!s_MaterialThumbnailFramebuffer || !s_MaterialThumbnailSphereMesh || !assetManager)
                return nullptr;

            // 确保材质与依赖贴图已加载。
            assetManager->GetAsset(materialHandle);

            const SceneLightingParameters previousLighting = Renderer3D::GetSceneLighting();
            const bool previousApplyDisplayEncoding = Renderer3D::GetApplyDisplayEncoding();
            Renderer3D::SetSceneLighting(BuildStudioLighting());
            Renderer3D::SetApplyDisplayEncoding(true);

            s_MaterialThumbnailFramebuffer->BindCapturingPrevious();
            RenderCommand::SetClearColor({MaterialThumbnailBackgroundRed, MaterialThumbnailBackgroundGreen,
                                          MaterialThumbnailBackgroundBlue, 1.0f});
            RenderCommand::Clear();

            Camera previewCamera(glm::perspective(glm::radians(32.0f), 1.0f, 0.05f, 50.0f));
            const glm::vec3 cameraEye{0.0f, 0.12f, 1.75f};
            const glm::mat4 cameraWorldTransform = glm::inverse(
                    glm::lookAt(cameraEye, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

            Renderer3D::BeginScene(previewCamera, cameraWorldTransform);
            const std::vector<AssetHandle> materialHandles{materialHandle};
            Renderer3D::DrawMeshAsset(s_MaterialThumbnailSphereMesh, materialHandles, glm::mat4(1.0f));
            Renderer3D::EndScene();

            std::vector<uint8_t> rgbaBytes;
            s_MaterialThumbnailFramebuffer->ReadColorPixels(0, rgbaBytes);
            s_MaterialThumbnailFramebuffer->UnbindRestoringPrevious();
            Renderer3D::SetApplyDisplayEncoding(previousApplyDisplayEncoding);
            Renderer3D::SetSceneLighting(previousLighting);

            if (rgbaBytes.size()
                != static_cast<size_t>(MaterialThumbnailPixelSize) * MaterialThumbnailPixelSize * 4u)
                return nullptr;

            FlipRgbaRowsVertically(rgbaBytes, MaterialThumbnailPixelSize, MaterialThumbnailPixelSize);

            Ref<Texture2D> thumbnailTexture =
                    Texture2D::Create(MaterialThumbnailPixelSize, MaterialThumbnailPixelSize);
            thumbnailTexture->SetData(rgbaBytes.data(), static_cast<uint32_t>(rgbaBytes.size()));
            return thumbnailTexture;
        }

        void EnqueueMaterialThumbnail(AssetHandle materialHandle)
        {
            const uint64_t cacheKey = static_cast<uint64_t>(materialHandle);
            if (s_PendingMaterialThumbnailKeys.find(cacheKey) != s_PendingMaterialThumbnailKeys.end())
                return;
            if (s_MaterialThumbnailCache.find(cacheKey) != s_MaterialThumbnailCache.end())
                return;

            s_PendingMaterialThumbnailKeys.insert(cacheKey);
            s_PendingMaterialThumbnailQueue.push_back(materialHandle);
        }
    }

    Ref<Texture2D> GetOrCreateMaterialThumbnail(AssetManager *assetManager, AssetHandle materialHandle)
    {
        if (materialHandle == 0 || !assetManager)
            return nullptr;

        const uint64_t cacheKey = static_cast<uint64_t>(materialHandle);
        const auto cacheIterator = s_MaterialThumbnailCache.find(cacheKey);
        if (cacheIterator != s_MaterialThumbnailCache.end())
            return cacheIterator->second;

        EnqueueMaterialThumbnail(materialHandle);
        return nullptr;
    }

    void ProcessPendingMaterialThumbnails(AssetManager *assetManager, uint32_t maxGenerationsPerFrame)
    {
        if (!assetManager || maxGenerationsPerFrame == 0)
            return;

        uint32_t generatedCount = 0;
        while (generatedCount < maxGenerationsPerFrame && !s_PendingMaterialThumbnailQueue.empty())
        {
            const AssetHandle materialHandle = s_PendingMaterialThumbnailQueue.front();
            s_PendingMaterialThumbnailQueue.pop_front();
            const uint64_t cacheKey = static_cast<uint64_t>(materialHandle);
            s_PendingMaterialThumbnailKeys.erase(cacheKey);

            if (s_MaterialThumbnailCache.find(cacheKey) != s_MaterialThumbnailCache.end())
                continue;

            if (!assetManager->IsAssetHandleValid(materialHandle))
                continue;

            Ref<Texture2D> thumbnailTexture = RenderMaterialThumbnail(assetManager, materialHandle);
            if (thumbnailTexture)
                s_MaterialThumbnailCache.emplace(cacheKey, thumbnailTexture);

            ++generatedCount;
        }
    }

    void InvalidateMaterialThumbnail(AssetHandle materialHandle)
    {
        const uint64_t cacheKey = static_cast<uint64_t>(materialHandle);
        s_MaterialThumbnailCache.erase(cacheKey);
        // 允许重新入队；若仍在队列中则保留，下一轮生成会覆盖缺失缓存。
    }

    void ClearMaterialThumbnailCache()
    {
        s_MaterialThumbnailCache.clear();
        s_PendingMaterialThumbnailQueue.clear();
        s_PendingMaterialThumbnailKeys.clear();
    }
}
