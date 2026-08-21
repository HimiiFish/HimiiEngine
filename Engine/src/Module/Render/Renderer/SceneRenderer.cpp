#include "Hepch.h"
#include "Module/Render/Renderer/SceneRenderer.h"
#include "World/Scene/Scene.h"
#include "World/Scene/Entity.h"
#include "World/Scene/Components.h"
#include "Resource/AssetManager.h"
#include "Resource/ResourceSystem.h"
#include "Project/Project.h"
#include "Module/Render/Renderer/Renderer2D.h"
#include "Module/Render/Renderer/Renderer3D.h"
#include "Module/Render/Renderer/EditorCamera.h"
#include "World/Scene/SceneCamera.h"
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/Renderer/SpriteRendererUtility.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Particle/ParticleSystem.h"
#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Environment/EnvironmentLightingSystem.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

namespace Himii
{
    namespace
    {
        struct SpriteDrawSortEntry
        {
            entt::entity EntityHandle{};
            TransformComponent *Transform = nullptr;
            SpriteRendererComponent *Sprite = nullptr;
        };

        constexpr uint32_t DirectionalCascadedShadowAtlasPaddingPixels = 2u;
        constexpr float DirectionalCascadedShadowSplitBlend = 0.85f;
        constexpr float DirectionalCascadedShadowOverlapRatio = 0.10f;
        constexpr float DirectionalCascadedShadowSphereEpsilon = 0.05f;
        constexpr float DefaultShadowBias = 0.0015f;

        struct DirectionalShadowParameters
        {
            bool Enabled = false;
            uint32_t ShadowMapResolutionPixels = 2048;
            glm::mat4 LightViewProjection[DirectionalCascadedShadowCascadeCount]{
                    glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f)};
            glm::vec4 CascadeSplitDistances{0.0f};
            glm::vec4 ShadowTexelWorldSize{0.0f};
            glm::vec3 ViewerForwardDirection{0.0f, 0.0f, -1.0f};
            float CascadeNearDistance = 0.05f;
        };

        struct ShadowViewerFrustum
        {
            glm::vec3 Position{0.0f};
            glm::vec3 ForwardDirection{0.0f, 0.0f, -1.0f};
            glm::vec3 RightDirection{1.0f, 0.0f, 0.0f};
            glm::vec3 UpDirection{0.0f, 1.0f, 0.0f};
            bool IsOrthographic = false;
            float NearClip = 0.1f;
            float FarClip = 1000.0f;
            float VerticalFieldOfViewRadians = glm::radians(45.0f);
            float OrthographicSize = 10.0f;
            float AspectRatio = 1.0f;
        };

        glm::vec3 GetTransformForwardDirection(const glm::mat4 &worldTransform)
        {
            const glm::vec3 forwardAxis = -glm::vec3(worldTransform[2]);
            const float lengthSquared = glm::dot(forwardAxis, forwardAxis);
            if (lengthSquared < 1e-8f)
                return glm::vec3(0.0f, -1.0f, 0.0f);
            return glm::normalize(forwardAxis);
        }

        glm::vec3 GetTransformRightDirection(const glm::mat4 &worldTransform)
        {
            const glm::vec3 rightAxis = glm::vec3(worldTransform[0]);
            const float lengthSquared = glm::dot(rightAxis, rightAxis);
            if (lengthSquared < 1e-8f)
                return glm::vec3(1.0f, 0.0f, 0.0f);
            return glm::normalize(rightAxis);
        }

        glm::vec3 GetTransformUpDirection(const glm::mat4 &worldTransform)
        {
            const glm::vec3 upAxis = glm::vec3(worldTransform[1]);
            const float lengthSquared = glm::dot(upAxis, upAxis);
            if (lengthSquared < 1e-8f)
                return glm::vec3(0.0f, 1.0f, 0.0f);
            return glm::normalize(upAxis);
        }

        glm::vec3 GetLightSpaceUpAxis(const glm::vec3 &lightTravelDirection)
        {
            glm::vec3 upAxis(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(lightTravelDirection, upAxis)) > 0.95f)
                upAxis = glm::vec3(0.0f, 0.0f, 1.0f);
            return upAxis;
        }

        float ExtractProjectionAspectRatio(const glm::mat4 &projection)
        {
            if (std::abs(projection[0][0]) < 1e-8f)
                return 1.0f;
            return projection[1][1] / projection[0][0];
        }

        ShadowViewerFrustum BuildShadowViewerFrustumFromSceneCamera(const SceneCamera &sceneCamera,
                                                                    const glm::mat4 &worldTransform)
        {
            ShadowViewerFrustum frustum;
            frustum.Position = glm::vec3(worldTransform[3]);
            frustum.ForwardDirection = GetTransformForwardDirection(worldTransform);
            frustum.RightDirection = GetTransformRightDirection(worldTransform);
            frustum.UpDirection = GetTransformUpDirection(worldTransform);
            frustum.IsOrthographic =
                    sceneCamera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic;
            if (frustum.IsOrthographic)
            {
                frustum.NearClip = sceneCamera.GetOrthographicNearClip();
                frustum.FarClip = sceneCamera.GetOrthographicFarClip();
                frustum.OrthographicSize = std::max(sceneCamera.GetOrthographicSize(), 0.1f);
            }
            else
            {
                frustum.NearClip = sceneCamera.GetPerspectiveNearClip();
                frustum.FarClip = sceneCamera.GetPerspectiveFarClip();
                frustum.VerticalFieldOfViewRadians = sceneCamera.GetPerspectiveVerticalFOV();
            }
            frustum.AspectRatio = ExtractProjectionAspectRatio(sceneCamera.GetProjection());
            return frustum;
        }

        ShadowViewerFrustum BuildShadowViewerFrustumFromEditorCamera(const EditorCamera &camera)
        {
            ShadowViewerFrustum frustum;
            frustum.Position = camera.GetPosition();
            frustum.ForwardDirection = camera.GetForwardDirection();
            frustum.RightDirection = camera.GetRightDirection();
            frustum.UpDirection = camera.GetUpDirection();
            frustum.IsOrthographic = camera.IsOrthographicProjection();
            frustum.NearClip = camera.GetNearClip();
            frustum.FarClip = camera.GetFarClip();
            if (frustum.IsOrthographic)
            {
                frustum.OrthographicSize = std::max(camera.GetDistance() * 2.0f, 0.1f);
            }
            else
            {
                const float projectionYy = camera.GetProjection()[1][1];
                frustum.VerticalFieldOfViewRadians =
                        2.0f * std::atan(1.0f / std::max(projectionYy, 1.0e-5f));
            }
            frustum.AspectRatio = ExtractProjectionAspectRatio(camera.GetProjection());
            return frustum;
        }

        glm::vec3 SnapShadowVolumeCenterToTexelGrid(const glm::vec3 &volumeCenter,
                                                    const glm::vec3 &lightTravelDirection,
                                                    float worldExtent, uint32_t tileResolutionPixels)
        {
            if (tileResolutionPixels == 0)
                return volumeCenter;

            const float texelWorldSize = worldExtent / static_cast<float>(tileResolutionPixels);
            if (texelWorldSize <= 0.0f)
                return volumeCenter;

            const glm::mat4 lightRotationView =
                    glm::lookAt(glm::vec3(0.0f), lightTravelDirection, GetLightSpaceUpAxis(lightTravelDirection));
            glm::vec3 centerInLightSpace = glm::vec3(lightRotationView * glm::vec4(volumeCenter, 1.0f));
            centerInLightSpace.x = std::floor(centerInLightSpace.x / texelWorldSize) * texelWorldSize;
            centerInLightSpace.y = std::floor(centerInLightSpace.y / texelWorldSize) * texelWorldSize;
            return glm::vec3(glm::inverse(lightRotationView) * glm::vec4(centerInLightSpace, 1.0f));
        }

        void ComputeFrustumSliceCorners(const ShadowViewerFrustum &frustum, float sliceNear, float sliceFar,
                                        glm::vec3 corners[8])
        {
            const auto writePlaneCorners = [&](float depth, glm::vec3 *destination)
            {
                const glm::vec3 planeCenter = frustum.Position + frustum.ForwardDirection * depth;
                float halfHeight = 0.0f;
                if (frustum.IsOrthographic)
                    halfHeight = frustum.OrthographicSize * 0.5f;
                else
                    halfHeight = std::tan(frustum.VerticalFieldOfViewRadians * 0.5f) * std::max(depth, 0.01f);
                const float halfWidth = halfHeight * frustum.AspectRatio;
                destination[0] = planeCenter - frustum.RightDirection * halfWidth - frustum.UpDirection * halfHeight;
                destination[1] = planeCenter + frustum.RightDirection * halfWidth - frustum.UpDirection * halfHeight;
                destination[2] = planeCenter + frustum.RightDirection * halfWidth + frustum.UpDirection * halfHeight;
                destination[3] = planeCenter - frustum.RightDirection * halfWidth + frustum.UpDirection * halfHeight;
            };

            writePlaneCorners(sliceNear, corners);
            writePlaneCorners(sliceFar, corners + 4);
        }

        float ComputePracticalSplitDistance(float nearDistance, float farDistance, uint32_t splitIndex,
                                            uint32_t cascadeCount)
        {
            const float splitRatio = static_cast<float>(splitIndex) / static_cast<float>(cascadeCount);
            const float uniformSplit = nearDistance + (farDistance - nearDistance) * splitRatio;
            const float safeNearDistance = std::max(nearDistance, 0.01f);
            const float logarithmicSplit =
                    safeNearDistance * std::pow(farDistance / safeNearDistance, splitRatio);
            return glm::mix(uniformSplit, logarithmicSplit, DirectionalCascadedShadowSplitBlend);
        }

        glm::mat4 BuildDirectionalLightViewProjectionFromSphere(const glm::vec3 &sphereCenter, float sphereRadius,
                                                                const glm::vec3 &lightTravelDirection,
                                                                float casterExtrusionDistance)
        {
            const float safeRadius = std::max(sphereRadius, 0.05f);
            const float halfExtent = safeRadius + DirectionalCascadedShadowSphereEpsilon;
            const float extrusionDistance = std::max(casterExtrusionDistance, halfExtent + 1.0f);
            const glm::vec3 eyePosition = sphereCenter - lightTravelDirection * extrusionDistance;
            const glm::mat4 lightView =
                    glm::lookAt(eyePosition, sphereCenter, GetLightSpaceUpAxis(lightTravelDirection));
            const float farDistance = extrusionDistance + halfExtent;
            const glm::mat4 lightProjection =
                    glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, 0.01f, farDistance);
            return lightProjection * lightView;
        }

        SceneLightingParameters GatherSceneLighting(Scene &scene)
        {
            static_assert(ScenePointLightCapacity == MaximumPointLightCount,
                          "Scene lighting UBO capacity must match LightComponent point-light limit.");

            SceneLightingParameters parameters{};

            auto lightView = scene.Registry().view<TransformComponent, LightComponent>();
            for (auto entityHandle : lightView)
            {
                const LightComponent &light = lightView.get<LightComponent>(entityHandle);
                if (!light.Enabled)
                    continue;

                if (light.Type == LightType::Directional)
                {
                    if (parameters.HasDirectionalLight)
                        continue;

                    const glm::mat4 worldTransform =
                            scene.GetEntityWorldTransformMatrix({entityHandle, &scene});
                    parameters.HasDirectionalLight = true;
                    parameters.DirectionalLightDirection = GetTransformForwardDirection(worldTransform);
                    parameters.DirectionalLightColor = glm::vec3(light.Color);
                    parameters.DirectionalLightIntensity = light.Intensity;
                    continue;
                }

                if (light.Type != LightType::Point)
                    continue;
                if (parameters.PointLightCount >= ScenePointLightCapacity)
                    continue;

                const glm::mat4 worldTransform =
                        scene.GetEntityWorldTransformMatrix({entityHandle, &scene});
                PointLightParameters &pointLight =
                        parameters.PointLights[parameters.PointLightCount++];
                pointLight.Position = glm::vec3(worldTransform[3]);
                pointLight.Range = std::max(light.Range, 0.01f);
                pointLight.Color = glm::vec3(light.Color);
                pointLight.Intensity = light.Intensity;
            }

            auto environmentView = scene.Registry().view<EnvironmentComponent>();
            for (auto entityHandle : environmentView)
            {
                const EnvironmentComponent &environment =
                        environmentView.get<EnvironmentComponent>(entityHandle);
                if (!environment.Enabled)
                    continue;

                parameters.AmbientColor = glm::vec3(environment.AmbientColor);
                parameters.AmbientIntensity = environment.AmbientIntensity;
                break;
            }

            if (!parameters.HasDirectionalLight)
            {
                parameters.AmbientColor = glm::vec3(0.0f);
                parameters.AmbientIntensity = 0.0f;
                parameters.HasShadowMap = false;
            }

            return parameters;
        }

        void ApplyEnvironmentImageBasedLighting(Scene &scene, SceneLightingParameters &lightingParameters)
        {
            EnvironmentLightingSystem::PollSourceChanges();

            AssetHandle environmentMapHandle = 0;
            float environmentIntensity = 1.0f;
            auto environmentView = scene.Registry().view<EnvironmentComponent>();
            for (auto entityHandle : environmentView)
            {
                const EnvironmentComponent &environment =
                        environmentView.get<EnvironmentComponent>(entityHandle);
                if (!environment.Enabled)
                    continue;
                environmentMapHandle = environment.EnvironmentMap;
                environmentIntensity = environment.Intensity;
                break;
            }

            if (environmentMapHandle == 0)
            {
                Renderer3D::ClearImageBasedLighting();
                lightingParameters.HasImageBasedLighting = false;
                return;
            }

            const BakedEnvironmentLighting baked =
                    EnvironmentLightingSystem::EnsureBaked(environmentMapHandle);
            if (!baked.Valid)
            {
                Renderer3D::ClearImageBasedLighting();
                lightingParameters.HasImageBasedLighting = false;
                return;
            }

            lightingParameters.HasImageBasedLighting = true;
            lightingParameters.EnvironmentIntensity = environmentIntensity;
            lightingParameters.PrefilterMipCount =
                    static_cast<float>(std::max(1u, baked.PrefilteredCubemap->GetMipLevelCount()));
            lightingParameters.AmbientColor = glm::vec3(0.0f);
            lightingParameters.AmbientIntensity = 0.0f;

            Renderer3D::SetImageBasedLighting(baked.IrradianceCubemap, baked.PrefilteredCubemap,
                                              baked.BrdfLookupTexture, environmentIntensity,
                                              lightingParameters.PrefilterMipCount);

            const bool isTwoDimensional =
                    Project::GetActive() && Project::GetActive()->GetConfig().Is2D;
            if (!isTwoDimensional && baked.EnvironmentCubemap)
                scene.SetSkybox(baked.EnvironmentCubemap);
        }

        DirectionalShadowParameters GatherDirectionalShadowParameters(Scene &scene,
                                                                     const ShadowViewerFrustum &viewerFrustum)
        {
            DirectionalShadowParameters parameters{};
            auto lightView = scene.Registry().view<TransformComponent, LightComponent>();
            for (auto entityHandle : lightView)
            {
                const LightComponent &light = lightView.get<LightComponent>(entityHandle);
                if (!light.Enabled || light.Type != LightType::Directional)
                    continue;

                if (!light.CastShadows)
                    return parameters;

                const glm::mat4 worldTransform =
                        scene.GetEntityWorldTransformMatrix({entityHandle, &scene});
                const glm::vec3 lightTravelDirection = GetTransformForwardDirection(worldTransform);
                const uint32_t atlasResolutionPixels =
                        GetShadowMapResolutionPixelCount(light.ShadowMapResolution);
                const uint32_t tileResolutionPixels = atlasResolutionPixels / 2u;
                const float maxShadowDistance = std::max(light.ShadowDistance, 0.1f);
                const float cascadeNearDistance = std::max(viewerFrustum.NearClip, 0.05f);
                const float cascadeFarDistance = std::min(viewerFrustum.FarClip, maxShadowDistance);
                // Shadow Distance 不超过相机近平面时，视锥级联没有正向覆盖范围。
                if (cascadeFarDistance <= cascadeNearDistance + 0.01f)
                    return parameters;

                float splitDistances[DirectionalCascadedShadowCascadeCount]{};
                for (uint32_t splitIndex = 1; splitIndex <= DirectionalCascadedShadowCascadeCount; ++splitIndex)
                {
                    splitDistances[splitIndex - 1] = ComputePracticalSplitDistance(
                            cascadeNearDistance, cascadeFarDistance, splitIndex,
                            DirectionalCascadedShadowCascadeCount);
                }
                splitDistances[DirectionalCascadedShadowCascadeCount - 1] = cascadeFarDistance;

                parameters.Enabled = true;
                parameters.ShadowMapResolutionPixels = atlasResolutionPixels;
                parameters.ViewerForwardDirection = viewerFrustum.ForwardDirection;
                parameters.CascadeNearDistance = cascadeNearDistance;
                parameters.CascadeSplitDistances = glm::vec4(splitDistances[0], splitDistances[1],
                                                             splitDistances[2], cascadeFarDistance);

                for (uint32_t cascadeIndex = 0; cascadeIndex < DirectionalCascadedShadowCascadeCount;
                     ++cascadeIndex)
                {
                    const float sliceNear =
                            cascadeIndex == 0 ? cascadeNearDistance : splitDistances[cascadeIndex - 1];
                    const float sliceFar = splitDistances[cascadeIndex];
                    const float sliceRange = std::max(sliceFar - sliceNear, 0.01f);
                    const float overlapExtend =
                            cascadeIndex + 1 < DirectionalCascadedShadowCascadeCount
                                    ? sliceRange * DirectionalCascadedShadowOverlapRatio
                                    : 0.0f;
                    const float renderFar = sliceFar + overlapExtend;

                    glm::vec3 corners[8]{};
                    ComputeFrustumSliceCorners(viewerFrustum, sliceNear, renderFar, corners);
                    glm::vec3 sphereCenter(0.0f);
                    for (uint32_t cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
                        sphereCenter += corners[cornerIndex];
                    sphereCenter /= 8.0f;

                    float sphereRadius = 0.0f;
                    for (uint32_t cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
                        sphereRadius = std::max(sphereRadius, glm::length(corners[cornerIndex] - sphereCenter));

                    const float worldExtent = (sphereRadius + DirectionalCascadedShadowSphereEpsilon) * 2.0f;
                    sphereCenter = SnapShadowVolumeCenterToTexelGrid(
                            sphereCenter, lightTravelDirection, worldExtent, tileResolutionPixels);

                    parameters.LightViewProjection[cascadeIndex] = BuildDirectionalLightViewProjectionFromSphere(
                            sphereCenter, sphereRadius, lightTravelDirection, maxShadowDistance);
                    parameters.ShadowTexelWorldSize[static_cast<int>(cascadeIndex)] =
                            worldExtent / static_cast<float>(std::max(tileResolutionPixels, 1u));
                }
                return parameters;
            }
            return parameters;
        }

        void DrawBuiltinMesh(const MeshComponent &mesh, const glm::mat4 &worldTransform, int entityIdentifier)
        {
            const AssetHandle materialHandle =
                    mesh.MaterialAssetHandles.empty() ? 0 : mesh.MaterialAssetHandles.front();
            Renderer3D::BuiltinLitPrimitive primitive = Renderer3D::BuiltinLitPrimitive::Cube;
            if (mesh.Type == MeshComponent::MeshType::Plane)
                primitive = Renderer3D::BuiltinLitPrimitive::Plane;
            else if (mesh.Type == MeshComponent::MeshType::Sphere)
                primitive = Renderer3D::BuiltinLitPrimitive::Sphere;
            else if (mesh.Type == MeshComponent::MeshType::Capsule)
                primitive = Renderer3D::BuiltinLitPrimitive::Capsule;

            Renderer3D::DrawBuiltinLitMesh(primitive, worldTransform, materialHandle, entityIdentifier);
        }

        void DrawSpriteRenderersSorted(Scene *scene, entt::registry &registry, AssetManager *assetManager)
        {
            if (!assetManager)
                return;

            std::vector<SpriteDrawSortEntry> drawEntries;
            auto view = registry.view<TransformComponent, SpriteRendererComponent>();
            for (auto entityHandle : view)
            {
                drawEntries.push_back({
                        entityHandle,
                        &view.get<TransformComponent>(entityHandle),
                        &view.get<SpriteRendererComponent>(entityHandle)});
            }

            std::stable_sort(drawEntries.begin(), drawEntries.end(),
                             [](const SpriteDrawSortEntry &left, const SpriteDrawSortEntry &right)
                             {
                                 if (left.Sprite->SortingLayer != right.Sprite->SortingLayer)
                                     return left.Sprite->SortingLayer < right.Sprite->SortingLayer;
                                 return left.Sprite->SortingOrder < right.Sprite->SortingOrder;
                             });

            for (const SpriteDrawSortEntry &entry : drawEntries)
            {
                Entity sceneEntity = {entry.EntityHandle, scene};
                const SpriteResolved resolved =
                        ResolveSpriteRendererDrawable(sceneEntity, *entry.Sprite, assetManager);
                Renderer2D::DrawSprite(scene->GetEntityWorldTransformMatrix(sceneEntity), *entry.Sprite, resolved,
                                       static_cast<int>(entry.EntityHandle));
            }
        }

        void DrawMeshComponents(Scene &scene)
        {
            auto meshView = scene.Registry().view<TransformComponent, MeshComponent>();
            meshView.each(
                    [&](entt::entity entityHandle, TransformComponent &, MeshComponent &mesh)
                    {
                        NormalizeMeshComponentMaterialSlots(mesh);
                        const glm::mat4 worldTransform =
                                scene.GetEntityWorldTransformMatrix({entityHandle, &scene});
                        if (mesh.Source == MeshComponent::MeshSource::Asset && mesh.MeshAssetHandle != 0)
                        {
                            auto assetManager = ResourceSystem::GetAssetManager();
                            if (!assetManager)
                                return;
                            Ref<Asset> meshBase = assetManager->GetAsset(mesh.MeshAssetHandle);
                            if (!meshBase || meshBase->GetType() != AssetType::Mesh)
                                return;
                            Ref<MeshAsset> meshAsset = std::static_pointer_cast<MeshAsset>(meshBase);
                            Renderer3D::DrawMeshAsset(meshAsset, mesh.MaterialAssetHandles, worldTransform,
                                                      (int)entityHandle);
                            return;
                        }

                        DrawBuiltinMesh(mesh, worldTransform, (int)entityHandle);
                    });
        }

        void RenderDirectionalShadowPass(Scene &scene, SceneLightingParameters &lightingParameters,
                                         const ShadowViewerFrustum &viewerFrustum)
        {
            const DirectionalShadowParameters shadowParameters =
                    GatherDirectionalShadowParameters(scene, viewerFrustum);
            if (!shadowParameters.Enabled || !lightingParameters.HasDirectionalLight)
            {
                lightingParameters.HasShadowMap = false;
                return;
            }

            Renderer3D::EnsureShadowMap(shadowParameters.ShadowMapResolutionPixels);
            Renderer3D::BeginShadowPass();
            const uint32_t atlasResolution = shadowParameters.ShadowMapResolutionPixels;
            const uint32_t tileResolution = atlasResolution / 2u;
            const uint32_t padding = DirectionalCascadedShadowAtlasPaddingPixels;
            for (uint32_t cascadeIndex = 0; cascadeIndex < DirectionalCascadedShadowCascadeCount; ++cascadeIndex)
            {
                const uint32_t viewportX = (cascadeIndex % 2u) * tileResolution + padding;
                const uint32_t viewportY = (cascadeIndex / 2u) * tileResolution + padding;
                const uint32_t viewportSize = tileResolution - padding * 2u;
                Renderer3D::SetShadowCascadeViewProjection(
                        shadowParameters.LightViewProjection[cascadeIndex], viewportX, viewportY,
                        viewportSize, viewportSize);
                DrawMeshComponents(scene);
            }
            Renderer3D::EndShadowPass();

            lightingParameters.HasShadowMap = true;
            for (uint32_t cascadeIndex = 0; cascadeIndex < DirectionalCascadedShadowCascadeCount; ++cascadeIndex)
                lightingParameters.LightViewProjection[cascadeIndex] =
                        shadowParameters.LightViewProjection[cascadeIndex];
            lightingParameters.ShadowBias = DefaultShadowBias;
            lightingParameters.CascadeSplitDistances = shadowParameters.CascadeSplitDistances;
            lightingParameters.ShadowTexelWorldSize = shadowParameters.ShadowTexelWorldSize;
            lightingParameters.ShadowViewerForwardDirection = shadowParameters.ViewerForwardDirection;
            lightingParameters.ShadowCascadeOverlapRatio = DirectionalCascadedShadowOverlapRatio;
            lightingParameters.ShadowCascadeNearDistance = shadowParameters.CascadeNearDistance;
            lightingParameters.ShadowAtlasTexelUvSize =
                    1.0f / static_cast<float>(std::max(atlasResolution, 1u));
        }
    }

    bool SceneRenderer::RenderGameWorld(Scene &scene, uint32_t targetWidth, uint32_t targetHeight)
    {
        if (targetWidth == 0 || targetHeight == 0)
            return false;

        scene.OnViewportResize(targetWidth, targetHeight);
        Entity cameraEntity = scene.GetPrimaryCameraEntity();
        if (!cameraEntity || !cameraEntity.HasComponent<TransformComponent>())
            return false;

        auto &cameraComponent = cameraEntity.GetComponent<CameraComponent>();
        const glm::mat4 cameraTransform = scene.GetEntityWorldTransformMatrix(cameraEntity);

        SceneLightingParameters lightingParameters = GatherSceneLighting(scene);
        ApplyEnvironmentImageBasedLighting(scene, lightingParameters);
        RenderDirectionalShadowPass(scene, lightingParameters,
                                    BuildShadowViewerFrustumFromSceneCamera(cameraComponent.Camera, cameraTransform));

        RenderCommand::SetDepthTest(true);
        Renderer3D::SetSceneLighting(lightingParameters);
        Renderer3D::BeginScene(cameraComponent.Camera, cameraTransform);
        const bool isTwoDimensional =
                Project::GetActive() && Project::GetActive()->GetConfig().Is2D;
        if (scene.m_SkyboxTexture && !isTwoDimensional)
            Renderer3D::DrawSkybox(
                    scene.m_SkyboxTexture, cameraComponent.Camera, cameraTransform);

        DrawMeshComponents(scene);
        Renderer3D::EndScene();

        RenderCommand::SetDepthTest(true);
        Renderer2D::BeginScene(cameraComponent.Camera, cameraTransform);
        {
            auto assetManager = ResourceSystem::GetAssetManager();
            DrawSpriteRenderersSorted(&scene, scene.m_Registry, assetManager.get());
        }

        if (ResourceSystem::IsBound())
        {
            auto assetManager = ResourceSystem::GetAssetManager();
            auto tilemapView = scene.m_Registry.view<TransformComponent, TilemapComponent>();
            tilemapView.each(
                    [&](entt::entity entityHandle, TransformComponent &, TilemapComponent &tilemap)
                    {
                        if (!assetManager || tilemap.TileMapHandle == 0)
                            return;
                        Ref<Asset> mapAsset = ResourceSystem::GetAsset(tilemap.TileMapHandle);
                        if (!mapAsset)
                            return;
                        Ref<TileMapData> mapData = std::static_pointer_cast<TileMapData>(mapAsset);
                        Ref<TileSet> tileSet;
                        if (mapData->GetTileSetHandle() != 0)
                        {
                            Ref<Asset> tileSetAsset =
                                    ResourceSystem::GetAsset(mapData->GetTileSetHandle());
                            if (tileSetAsset)
                                tileSet = std::static_pointer_cast<TileSet>(tileSetAsset);
                        }
                        Renderer2D::DrawTilemap(
                                scene.GetEntityWorldTransformMatrix({entityHandle, &scene}),
                                mapData, tileSet, (int)entityHandle);
                    });
        }

        auto circleView = scene.m_Registry.view<TransformComponent, CircleRendererComponent>();
        circleView.each(
                [&](entt::entity entityHandle, TransformComponent &, CircleRendererComponent &circle)
                {
                    Renderer2D::DrawCircle(
                            scene.GetEntityWorldTransformMatrix({entityHandle, &scene}),
                            circle.Color, circle.Thickness, circle.Fade, (int)entityHandle);
                });

        auto particleAssetManager = ResourceSystem::GetAssetManager();
        scene.m_ParticleSystem.ForEachAlive(
                [&](const ParticleSystem::ParticleView &particle)
                {
                    const float lifetimeProgress =
                            1.0f - particle.remainingLife / particle.lifetime;
                    const glm::vec4 color =
                            glm::mix(particle.colorBegin, particle.colorEnd, lifetimeProgress);
                    const float size =
                            glm::mix(particle.sizeBegin, particle.sizeEnd, lifetimeProgress);
                    const glm::mat4 transform =
                            glm::translate(glm::mat4(1.0f), particle.position)
                            * glm::rotate(
                                    glm::mat4(1.0f), particle.rotation,
                                    glm::vec3(0.0f, 0.0f, 1.0f))
                            * glm::scale(glm::mat4(1.0f), glm::vec3(size));

                    Ref<Texture2D> texture;
                    if (particle.textureHandle != 0 && particleAssetManager
                        && ResourceSystem::IsAssetHandleValid(
                                static_cast<AssetHandle>(particle.textureHandle)))
                    {
                        texture = std::dynamic_pointer_cast<Texture2D>(
                                ResourceSystem::GetAsset(
                                        static_cast<AssetHandle>(particle.textureHandle)));
                    }

                    if (particle.shape == ParticleShape::Circle)
                        Renderer2D::DrawCircle(transform, color, 1.0f, 0.0025f);
                    else if (texture)
                        Renderer2D::DrawQuad(transform, texture, 1.0f, color);
                    else
                        Renderer2D::DrawQuad(transform, color);
                });
        Renderer2D::EndScene();
        return true;
    }

    void SceneRenderer::RenderWorld(Scene &scene, EditorCamera &camera)
    {
        Renderer2D::BeginScene(camera);

        {
            auto assetManager = ResourceSystem::GetAssetManager();
            DrawSpriteRenderersSorted(&scene, scene.m_Registry, assetManager.get());
        }

        if (ResourceSystem::IsBound())
        {
            auto assetManager = ResourceSystem::GetAssetManager();
            auto view = scene.m_Registry.view<TransformComponent, TilemapComponent>();
            view.each([&](entt::entity entityHandle, TransformComponent &, TilemapComponent &tilemap)
                      {
                          if (!assetManager || tilemap.TileMapHandle == 0)
                              return;

                          auto mapAsset = ResourceSystem::GetAsset(tilemap.TileMapHandle);
                          if (!mapAsset)
                              return;
                          auto mapData = std::static_pointer_cast<TileMapData>(mapAsset);

                          Ref<TileSet> tileSet = nullptr;
                          if (mapData->GetTileSetHandle() != 0)
                          {
                              auto tileSetAsset = ResourceSystem::GetAsset(mapData->GetTileSetHandle());
                              if (tileSetAsset)
                                  tileSet = std::static_pointer_cast<TileSet>(tileSetAsset);
                          }

                          Renderer2D::DrawTilemap(scene.GetEntityWorldTransformMatrix({entityHandle, &scene}),
                                                  mapData, tileSet, (int)entityHandle);
                      });
        }

        {
            auto view = scene.m_Registry.view<TransformComponent, CircleRendererComponent>();
            view.each(
                    [&](entt::entity entityHandle, TransformComponent &, CircleRendererComponent &circle)
                    {
                        Renderer2D::DrawCircle(scene.GetEntityWorldTransformMatrix({entityHandle, &scene}),
                                               circle.Color, circle.Thickness, circle.Fade, (int)entityHandle);
                    });
        }

        Renderer2D::EndScene();

        {
            SceneLightingParameters lightingParameters = GatherSceneLighting(scene);
            ApplyEnvironmentImageBasedLighting(scene, lightingParameters);
            RenderDirectionalShadowPass(scene, lightingParameters,
                                        BuildShadowViewerFrustumFromEditorCamera(camera));
            Renderer3D::SetSceneLighting(lightingParameters);
            Renderer3D::BeginScene(camera);

            bool isTwoDimensional = false;
            if (Project::GetActive())
                isTwoDimensional = Project::GetActive()->GetConfig().Is2D;

            if (scene.m_SkyboxTexture && !isTwoDimensional)
                Renderer3D::DrawSkybox(scene.m_SkyboxTexture, camera);

            DrawMeshComponents(scene);
            Renderer3D::EndScene();
        }
    }
}
