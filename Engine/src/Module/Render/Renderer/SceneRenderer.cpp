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
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Render/Renderer/SpriteRendererUtility.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Particle/ParticleSystem.h"
#include "Module/Render/Mesh/MeshAsset.h"
#include "Module/Render/Mesh/MaterialAsset.h"

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

        struct BuiltinSurfaceParameters
        {
            glm::vec4 AlbedoColor{1.0f};
            float Specular = 0.5f;
            float Shininess = 32.0f;
            Ref<Texture2D> AlbedoTexture;
        };

        struct DirectionalShadowParameters
        {
            bool Enabled = false;
            glm::mat4 LightViewProjection{1.0f};
            uint32_t ShadowMapResolutionPixels = 2048;
            float ShadowTexelWorldSize = 0.0f;
        };

        /// 观察者（Editor 相机或 Runtime 主相机）位置与朝向；阴影盒中心由它决定。
        struct ShadowViewerAnchor
        {
            glm::vec3 Position{0.0f};
            glm::vec3 ForwardDirection{0.0f, 0.0f, -1.0f};
        };

        constexpr float DefaultShadowBias = 0.0015f;

        glm::vec3 GetTransformForwardDirection(const glm::mat4 &worldTransform)
        {
            const glm::vec3 forwardAxis = -glm::vec3(worldTransform[2]);
            const float lengthSquared = glm::dot(forwardAxis, forwardAxis);
            if (lengthSquared < 1e-8f)
                return glm::vec3(0.0f, -1.0f, 0.0f);
            return glm::normalize(forwardAxis);
        }

        glm::vec3 GetLightSpaceUpAxis(const glm::vec3 &lightTravelDirection)
        {
            glm::vec3 upAxis(0.0f, 1.0f, 0.0f);
            if (std::abs(glm::dot(lightTravelDirection, upAxis)) > 0.95f)
                upAxis = glm::vec3(0.0f, 0.0f, 1.0f);
            return upAxis;
        }

        /// 正交阴影体积中心跟随观察点：沿视线前移半个 ShadowSize，让视野中心落在阴影覆盖区内。
        /// 方向光实体的位置不参与，只有其朝向生效。
        glm::vec3 ComputeShadowVolumeCenter(const ShadowViewerAnchor &viewerAnchor, float shadowSize)
        {
            return viewerAnchor.Position + viewerAnchor.ForwardDirection * (shadowSize * 0.5f);
        }

        /// 把阴影盒中心对齐到光空间的贴图 texel 网格，避免相机移动时阴影边缘逐帧抖动。
        glm::vec3 SnapShadowVolumeCenterToTexelGrid(const glm::vec3 &volumeCenter,
                                                    const glm::vec3 &lightTravelDirection,
                                                    float shadowSize, uint32_t shadowMapResolutionPixels)
        {
            if (shadowMapResolutionPixels == 0)
                return volumeCenter;

            const float texelWorldSize = shadowSize / static_cast<float>(shadowMapResolutionPixels);
            if (texelWorldSize <= 0.0f)
                return volumeCenter;

            const glm::mat4 lightRotationView =
                    glm::lookAt(glm::vec3(0.0f), lightTravelDirection, GetLightSpaceUpAxis(lightTravelDirection));
            glm::vec3 centerInLightSpace = glm::vec3(lightRotationView * glm::vec4(volumeCenter, 1.0f));
            centerInLightSpace.x = std::floor(centerInLightSpace.x / texelWorldSize) * texelWorldSize;
            centerInLightSpace.y = std::floor(centerInLightSpace.y / texelWorldSize) * texelWorldSize;
            return glm::vec3(glm::inverse(lightRotationView) * glm::vec4(centerInLightSpace, 1.0f));
        }

        /// ShadowSize 为正交宽/高，ShadowDistance 为沿光方向近远跨度。
        glm::mat4 BuildDirectionalLightViewProjection(const glm::vec3 &volumeCenter,
                                                      const glm::vec3 &lightTravelDirection,
                                                      float shadowSize, float shadowDistance)
        {
            const float safeShadowSize = std::max(shadowSize, 0.1f);
            const float safeShadowDistance = std::max(shadowDistance, 0.1f);
            const float halfExtent = safeShadowSize * 0.5f;

            const glm::vec3 eyePosition =
                    volumeCenter - lightTravelDirection * (safeShadowDistance * 0.5f);

            const glm::mat4 lightView =
                    glm::lookAt(eyePosition, volumeCenter, GetLightSpaceUpAxis(lightTravelDirection));
            const glm::mat4 lightProjection = glm::ortho(
                    -halfExtent, halfExtent, -halfExtent, halfExtent, 0.01f, safeShadowDistance);
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

        DirectionalShadowParameters GatherDirectionalShadowParameters(Scene &scene,
                                                                     const ShadowViewerAnchor &viewerAnchor)
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
                const float safeShadowSize = std::max(light.ShadowSize, 0.1f);
                const uint32_t resolutionPixels =
                        GetShadowMapResolutionPixelCount(light.ShadowMapResolution);

                const glm::vec3 volumeCenter = SnapShadowVolumeCenterToTexelGrid(
                        ComputeShadowVolumeCenter(viewerAnchor, safeShadowSize), lightTravelDirection,
                        safeShadowSize, resolutionPixels);

                parameters.Enabled = true;
                parameters.LightViewProjection = BuildDirectionalLightViewProjection(
                        volumeCenter, lightTravelDirection, safeShadowSize, light.ShadowDistance);
                parameters.ShadowMapResolutionPixels = resolutionPixels;
                parameters.ShadowTexelWorldSize = safeShadowSize / static_cast<float>(resolutionPixels);
                return parameters;
            }
            return parameters;
        }

        BuiltinSurfaceParameters ResolveBuiltinSurface(const MeshComponent &mesh)
        {
            BuiltinSurfaceParameters surface;
            surface.AlbedoColor = mesh.Color;

            if (mesh.MaterialAssetHandles.empty())
                return surface;

            auto assetManager = ResourceSystem::GetAssetManager();
            if (!assetManager)
                return surface;

            const AssetHandle materialHandle = mesh.MaterialAssetHandles.front();
            if (materialHandle == 0)
                return surface;

            Ref<Asset> materialBase = assetManager->GetAsset(materialHandle);
            if (!materialBase || materialBase->GetType() != AssetType::Material)
                return surface;

            Ref<MaterialAsset> materialAsset = std::static_pointer_cast<MaterialAsset>(materialBase);
            surface.AlbedoColor = materialAsset->AlbedoColor;
            surface.Specular = materialAsset->Specular;
            surface.Shininess = materialAsset->Shininess;
            if (materialAsset->AlbedoTextureHandle != 0)
            {
                Ref<Asset> textureBase = assetManager->GetAsset(materialAsset->AlbedoTextureHandle);
                if (textureBase && textureBase->GetType() == AssetType::Texture2D)
                    surface.AlbedoTexture = std::static_pointer_cast<Texture2D>(textureBase);
            }
            return surface;
        }

        void DrawBuiltinMesh(const MeshComponent &mesh, const glm::mat4 &worldTransform, int entityIdentifier)
        {
            const BuiltinSurfaceParameters surface = ResolveBuiltinSurface(mesh);
            if (mesh.Type == MeshComponent::MeshType::Cube)
                Renderer3D::DrawCube(worldTransform, surface.AlbedoColor, entityIdentifier, surface.Specular,
                                     surface.Shininess, surface.AlbedoTexture);
            else if (mesh.Type == MeshComponent::MeshType::Plane)
                Renderer3D::DrawPlane(worldTransform, surface.AlbedoColor, entityIdentifier, surface.Specular,
                                      surface.Shininess, surface.AlbedoTexture);
            else if (mesh.Type == MeshComponent::MeshType::Sphere)
                Renderer3D::DrawSphere(worldTransform, surface.AlbedoColor, entityIdentifier, surface.Specular,
                                       surface.Shininess, surface.AlbedoTexture);
            else if (mesh.Type == MeshComponent::MeshType::Capsule)
                Renderer3D::DrawCapsule(worldTransform, surface.AlbedoColor, entityIdentifier, surface.Specular,
                                        surface.Shininess, surface.AlbedoTexture);
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
                                                      mesh.Color, (int)entityHandle);
                            return;
                        }

                        DrawBuiltinMesh(mesh, worldTransform, (int)entityHandle);
                    });
        }

        void RenderDirectionalShadowPass(Scene &scene, SceneLightingParameters &lightingParameters,
                                         const ShadowViewerAnchor &viewerAnchor)
        {
            const DirectionalShadowParameters shadowParameters =
                    GatherDirectionalShadowParameters(scene, viewerAnchor);
            if (!shadowParameters.Enabled || !lightingParameters.HasDirectionalLight)
            {
                lightingParameters.HasShadowMap = false;
                return;
            }

            Renderer3D::EnsureShadowMap(shadowParameters.ShadowMapResolutionPixels);
            Renderer3D::BeginShadowPass(shadowParameters.LightViewProjection);
            DrawMeshComponents(scene);
            Renderer3D::EndShadowPass();

            lightingParameters.HasShadowMap = true;
            lightingParameters.LightViewProjection = shadowParameters.LightViewProjection;
            lightingParameters.ShadowBias = DefaultShadowBias;
            lightingParameters.ShadowTexelWorldSize = shadowParameters.ShadowTexelWorldSize;
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
        ShadowViewerAnchor viewerAnchor;
        viewerAnchor.Position = glm::vec3(cameraTransform[3]);
        viewerAnchor.ForwardDirection = GetTransformForwardDirection(cameraTransform);
        RenderDirectionalShadowPass(scene, lightingParameters, viewerAnchor);

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
            ShadowViewerAnchor viewerAnchor;
            viewerAnchor.Position = camera.GetPosition();
            viewerAnchor.ForwardDirection = camera.GetForwardDirection();
            RenderDirectionalShadowPass(scene, lightingParameters, viewerAnchor);
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
