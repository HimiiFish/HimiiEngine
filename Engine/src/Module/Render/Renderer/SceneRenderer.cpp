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

        glm::vec3 GetTransformForwardDirection(const glm::mat4 &worldTransform)
        {
            const glm::vec3 forwardAxis = -glm::vec3(worldTransform[2]);
            const float lengthSquared = glm::dot(forwardAxis, forwardAxis);
            if (lengthSquared < 1e-8f)
                return glm::vec3(0.0f, -1.0f, 0.0f);
            return glm::normalize(forwardAxis);
        }

        SceneLightingParameters GatherSceneLighting(Scene &scene)
        {
            SceneLightingParameters parameters{};

            auto lightView = scene.Registry().view<TransformComponent, LightComponent>();
            for (auto entityHandle : lightView)
            {
                const LightComponent &light = lightView.get<LightComponent>(entityHandle);
                if (!light.Enabled || light.Type != LightType::Directional)
                    continue;

                const glm::mat4 worldTransform =
                        scene.GetEntityWorldTransformMatrix({entityHandle, &scene});
                parameters.HasDirectionalLight = true;
                parameters.DirectionalLightDirection = GetTransformForwardDirection(worldTransform);
                parameters.DirectionalLightColor = glm::vec3(light.Color);
                parameters.DirectionalLightIntensity = light.Intensity;
                break;
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

        RenderCommand::SetDepthTest(true);
        Renderer3D::SetSceneLighting(GatherSceneLighting(scene));
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
            Renderer3D::SetSceneLighting(GatherSceneLighting(scene));
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
