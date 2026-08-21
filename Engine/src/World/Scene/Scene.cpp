#include "Scene.h"
#include "Entity.h"
#include "Hepch.h"

#include "Components.h"
#include "Module/Animation/SpriteAnimationUtility.h"
#include "Module/Animation/SpriteAnimationSystem.h"
#include "Module/Particle/ParticleEmitterSystem.h"
#include "Project/Project.h"
#include "Module/Render/Renderer/Renderer2D.h"
#include "Module/Render/Renderer/Renderer3D.h"
#include "Module/Render/RHI/RenderCommand.h"
#include "Module/Audio/AudioEngine.h"
#include "Module/Audio/SoundPlayerUtility.h"
#include "Module/Render/Renderer/Font.h"
#include "Module/Particle/ParticleSystem.h"
#include "Module/Script/ScriptEngine.h"
#include "ScriptableEntity.h"
#include "World/Scene/SceneInternal.h"
#include "EngineCore/Core/Log.h"
#include "World/World.h"
#include "Module/Physics/Physics2DWorld.h"
#include "Module/Render/Renderer/SceneRenderer.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace Himii
{
    Scene::Scene() = default;

    Scene::~Scene()
    {
        m_OwningWorld = nullptr;
    }

    WorldModuleRegistry &Scene::GetWorldModuleRegistry()
    {
        HIMII_CORE_ASSERT(m_OwningWorld, "Scene is not bound to a World");
        return m_OwningWorld->GetModuleRegistry();
    }

    Scene::RaycastHit2D Scene::Raycast2D(glm::vec2 start, glm::vec2 end)
    {
        if (m_OwningWorld)
        {
            if (Physics2DWorld *physics2DWorld = m_OwningWorld->GetPhysics2DWorld())
                return physics2DWorld->Raycast2D(start, end);
        }
        RaycastHit2D hit;
        hit.Hit = false;
        return hit;
    }

    void Scene::SyncEntityTransformToPhysics(Entity entity)
    {
        if (m_OwningWorld)
        {
            if (Physics2DWorld *physics2DWorld = m_OwningWorld->GetPhysics2DWorld())
                physics2DWorld->SyncEntityTransform(entity);
        }
    }
    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
    {
        Entity entity(m_Registry.create(), this);

        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;

        return entity;
    }

    Entity Scene::CreateEntity(const std::string &name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateUIEntity(const std::string &name)
    {
        return CreateUIEntityWithUUID(UUID(),name);
    }

    Entity Scene::CreateUIEntityWithUUID(UUID uuid, const std::string &name)
    {
        Entity entity(m_Registry.create(), this);

        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<RectTransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;

        return entity;
    }

    Entity Scene::CreateCanvasEntity(const std::string &name)
    {
        if (FindCanvasEntity())
        {
            HIMII_CORE_WARNING("Scene already has a Canvas; only one Canvas is allowed.");
            return {};
        }

        Entity canvasEntity = CreateUIEntity(name.empty() ? "Canvas" : name);
        auto& canvas = canvasEntity.AddComponent<CanvasComponent>();
        canvas.ReferenceResolution = {1920.0f, 1080.0f};
        canvas.MatchWidthOrHeight = 0.5f;
        SyncCanvasReferenceResolutionToTransform(canvasEntity);
        return canvasEntity;
    }

    Entity Scene::CreateUIButtonEntity(const std::string &name)
    {
        Entity buttonEntity = CreateUIEntity(name.empty() ? "Button" : name);
        auto& userInterfaceTransform = buttonEntity.GetComponent<RectTransformComponent>();
        userInterfaceTransform.SizeDelta = {160.0f, 40.0f};
        userInterfaceTransform.ResolvedSize = userInterfaceTransform.SizeDelta;

        auto& image = buttonEntity.AddComponent<UIImageComponent>();
        image.Color = {1.0f, 1.0f, 1.0f, 1.0f};
        buttonEntity.AddComponent<UIButtonComponent>();
        return buttonEntity;
    }
    void Scene::DestroyEntity(entt::entity entityHandle)
    {
        if (!m_Registry.valid(entityHandle))
            return;

        Entity entity{entityHandle, this};
        const std::vector<UUID> childIdentifiers = GetEntityChildren(entity);
        for (UUID childIdentifier : childIdentifiers)
        {
            Entity childEntity = GetEntityByUUID(childIdentifier);
            if (childEntity)
                DestroyEntity((entt::entity)childEntity);
        }

        if (auto *identifierComponent = m_Registry.try_get<IDComponent>(entityHandle))
        {
            auto identifierIterator = m_EntityMap.find(identifierComponent->ID);
            if (identifierIterator != m_EntityMap.end())
                m_EntityMap.erase(identifierIterator);
        }
        // 脚本析构
        if (NativeScriptComponent *nativeScriptComponent = m_Registry.try_get<NativeScriptComponent>(entityHandle))
        {
            if (nativeScriptComponent->Instance)
            {
                nativeScriptComponent->Instance->OnDestroy();
            }
            if (nativeScriptComponent->DestroyScript)
            {
                nativeScriptComponent->DestroyScript(nativeScriptComponent);
            }
        }
        m_Registry.destroy(entityHandle);
        RebuildHierarchyCache();
    }

    void Scene::ClearEntities()
    {
        std::vector<entt::entity> entityHandles;
        m_Registry.view<IDComponent>().each(
            [&](entt::entity entityHandle, IDComponent&)
            {
                entityHandles.push_back(entityHandle);
            });

        for (entt::entity entityHandle : entityHandles)
            DestroyEntity(entityHandle);
    }

    void Scene::OnRuntimeStart()
    {
        ScriptEngine::OnRuntimeStart(this);
        GetWorldModuleRegistry().RuntimeStartAll();

        {
            auto animationView = m_Registry.view<SpriteAnimationComponent>();
            for (auto entityHandle : animationView)
            {
                Entity entity = {entityHandle, this};
                ResetSpriteAnimationPlayback(entity.GetComponent<SpriteAnimationComponent>());
            }
        }
        {
            auto view = m_Registry.view<ScriptComponent>();
            for (auto e: view)
            {
                Entity entity = {e, this};
                ScriptEngine::OnCreateEntity(entity);
            }
        }

        {
            auto soundView = m_Registry.view<SoundPlayerComponent>();
            for (auto entityHandle : soundView)
            {
                auto& soundPlayer = soundView.get<SoundPlayerComponent>(entityHandle);
                soundPlayer.RuntimeVoiceHandle = AudioEngine::InvalidVoiceHandle;
                soundPlayer.RuntimePaused = false;
                if (soundPlayer.PlayOnStart)
                    SoundPlayerUtility::Play(soundPlayer);
            }
        }
    }

    void Scene::OnSimulationStart()
    {
        GetWorldModuleRegistry().RuntimeStartAll();
    }

    void Scene::OnSimulationStop()
    {
        GetWorldModuleRegistry().RuntimeStopAll();
    }

    void Scene::OnRuntimeStop()
    {
        SoundPlayerUtility::StopAllPlayersInScene(this);
        GetWorldModuleRegistry().RuntimeStopAll();
        ScriptEngine::OnRuntimeStop();
    }

    void Scene::OnUpdateEditor(Timestep ts, EditorCamera &camera, bool drawUserInterfaceContent)
    {
        UpdateSpriteAnimations(ts, true);
        RenderEditorView(camera, drawUserInterfaceContent);
    }

    void Scene::RenderEditorView(EditorCamera& camera, bool drawUserInterfaceContent)
    {
        SceneRenderer::RenderWorld(*this, camera);
        RenderUIInEditor(camera, drawUserInterfaceContent);
    }

    bool Scene::RenderGameView(
            uint32_t targetWidth, uint32_t targetHeight,
            bool drawUserInterfaceContent)
    {
        if (!SceneRenderer::RenderGameWorld(*this, targetWidth, targetHeight))
            return false;
        if (drawUserInterfaceContent)
            RenderGameUserInterface(targetWidth, targetHeight);
        return true;
    }

    void Scene::UpdateSpriteAnimations(Timestep timestep, bool allowEditorPreview)
    {
        SpriteAnimationSystem::Update(*this, timestep, allowEditorPreview);
    }

    void Scene::RunScriptFixedUpdate(Timestep ts)
    {
        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, this};
            ScriptEngine::OnFixedUpdateScript(entity, ts);
        }
    }

    void Scene::UpdateManagedAndNativeScripts(Timestep ts)
    {
        auto view = m_Registry.view<ScriptComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, this};
            ScriptEngine::OnUpdateScript(entity, ts);
        }

        m_Registry.view<NativeScriptComponent>().each(
                [=](auto entity, auto &nativeScriptComponent)
                {
                    if (!nativeScriptComponent.Instance)
                    {
                        nativeScriptComponent.Instance = nativeScriptComponent.InstantiateScript();
                        nativeScriptComponent.Instance->m_Entity = Entity{entity, this};
                        nativeScriptComponent.Instance->OnCreate();
                    }

                    nativeScriptComponent.Instance->OnUpdate(ts);
                });
    }

    void Scene::UpdateParticleEmittersAndSystem(Timestep ts)
    {
        ParticleEmitterSystem::UpdateEmittersAndSimulate(*this, m_ParticleSystem, ts);
    }

    void Scene::OnUpdateRuntime(Timestep ts, bool drawUserInterfaceContent)
    {
        if (m_OwningWorld)
            m_OwningWorld->OnUpdateRuntime(ts, drawUserInterfaceContent);
    }

    void Scene::OnUpdateSimulation(Timestep ts, EditorCamera &camera)
    {
        if (m_OwningWorld)
            m_OwningWorld->OnUpdateSimulation(ts, camera);
    }

    void Scene::RenderSimulationView(EditorCamera &camera)
    {
        SceneRenderer::RenderWorld(*this, camera);
    }

    template<typename Component>
    static void CopyComponent(entt::registry &dst, entt::registry &src,
                              const std::unordered_map<UUID, entt::entity> &enttMap)
    {
        auto view = src.view<Component>();
        for (auto e: view)
        {
            UUID uuid = src.get<IDComponent>(e).ID;
            // Find target entity by UUID
            if (enttMap.find(uuid) == enttMap.end())
                continue;

            entt::entity dstEnttID = enttMap.at(uuid);
            if constexpr (std::is_same_v<Component, TransformComponent>)
            {
                if (dst.all_of<RectTransformComponent>(dstEnttID))
                    continue;
            }
            else if constexpr (std::is_same_v<Component, RectTransformComponent>)
            {
                if (dst.all_of<TransformComponent>(dstEnttID))
                    continue;
            }
            auto &component = src.get<Component>(e);
            dst.emplace_or_replace<Component>(dstEnttID, component);
        }
    }

    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        newScene->m_ViewportWidth = other->m_ViewportWidth;
        newScene->m_ViewportHeight = other->m_ViewportHeight;

        newScene->m_SkyboxTexture = other->m_SkyboxTexture;

        auto &srcSceneRegistry = other->m_Registry;
        auto &dstSceneRegistry = newScene->m_Registry;
        std::unordered_map<UUID, entt::entity> enttMap;

        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e: idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto &name = srcSceneRegistry.get<TagComponent>(e).Tag;
            const bool isUserInterfaceEntity = srcSceneRegistry.all_of<RectTransformComponent>(e);
            Entity newEntity = isUserInterfaceEntity ? newScene->CreateUIEntityWithUUID(uuid, name)
                                                     : newScene->CreateEntityWithUUID(uuid, name);
            enttMap[uuid] = (entt::entity)newEntity;
        }

        // Copy components
        CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<ScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<Rigidbody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteAnimationComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<MeshComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<TilemapComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<TilemapCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<ParticleEmitterComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        //UI
        CopyComponent<RelationshipComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RectTransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CanvasComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<UIImageComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<UITextComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<UIButtonComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SoundPlayerComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

        newScene->RebuildHierarchyCache();

        return newScene;
    }

    Entity Scene::DuplicateEntity(Entity entity)
    {
        std::string name = entity.GetName();
        const bool isUserInterfaceEntity = entity.HasComponent<RectTransformComponent>();
        Entity newEntity = isUserInterfaceEntity ? CreateUIEntity(name) : CreateEntity(name);

        if (!isUserInterfaceEntity && entity.HasComponent<TransformComponent>())
            newEntity.GetComponent<TransformComponent>() = entity.GetComponent<TransformComponent>();

        if (entity.HasComponent<SpriteRendererComponent>())
            newEntity.AddComponent<SpriteRendererComponent>(entity.GetComponent<SpriteRendererComponent>());

        if (entity.HasComponent<CircleRendererComponent>())
            newEntity.AddComponent<CircleRendererComponent>(entity.GetComponent<CircleRendererComponent>());

        if (entity.HasComponent<CameraComponent>())
            newEntity.AddComponent<CameraComponent>(entity.GetComponent<CameraComponent>());

        if (entity.HasComponent<NativeScriptComponent>())
            newEntity.AddComponent<NativeScriptComponent>(entity.GetComponent<NativeScriptComponent>());

        if (entity.HasComponent<ScriptComponent>())
            newEntity.AddComponent<ScriptComponent>(entity.GetComponent<ScriptComponent>());

        if (entity.HasComponent<Rigidbody2DComponent>())
            newEntity.AddComponent<Rigidbody2DComponent>(entity.GetComponent<Rigidbody2DComponent>());

        if (entity.HasComponent<BoxCollider2DComponent>())
            newEntity.AddComponent<BoxCollider2DComponent>(entity.GetComponent<BoxCollider2DComponent>());

        if (entity.HasComponent<CircleCollider2DComponent>())
            newEntity.AddComponent<CircleCollider2DComponent>(entity.GetComponent<CircleCollider2DComponent>());

        if (entity.HasComponent<SpriteAnimationComponent>())
            newEntity.AddComponent<SpriteAnimationComponent>(entity.GetComponent<SpriteAnimationComponent>());

        if (entity.HasComponent<MeshComponent>())
            newEntity.AddComponent<MeshComponent>(entity.GetComponent<MeshComponent>());

        if (entity.HasComponent<LightComponent>())
            newEntity.AddComponent<LightComponent>(entity.GetComponent<LightComponent>());

        if (entity.HasComponent<EnvironmentComponent>())
            newEntity.AddComponent<EnvironmentComponent>(entity.GetComponent<EnvironmentComponent>());

        if (entity.HasComponent<TilemapComponent>())
            newEntity.AddComponent<TilemapComponent>(entity.GetComponent<TilemapComponent>());
        if (entity.HasComponent<TilemapCollider2DComponent>())
            newEntity.AddComponent<TilemapCollider2DComponent>(
                    entity.GetComponent<TilemapCollider2DComponent>());
        if (entity.HasComponent<ParticleEmitterComponent>())
            newEntity.AddComponent<ParticleEmitterComponent>(entity.GetComponent<ParticleEmitterComponent>());

        if (isUserInterfaceEntity && entity.HasComponent<RectTransformComponent>())
            newEntity.GetComponent<RectTransformComponent>() = entity.GetComponent<RectTransformComponent>();
        if (entity.HasComponent<CanvasComponent>())
            newEntity.AddComponent<CanvasComponent>(entity.GetComponent<CanvasComponent>());
        if (entity.HasComponent<UIImageComponent>())
            newEntity.AddComponent<UIImageComponent>(entity.GetComponent<UIImageComponent>());
        if (entity.HasComponent<UITextComponent>())
            newEntity.AddComponent<UITextComponent>(entity.GetComponent<UITextComponent>());
        if (entity.HasComponent<UIButtonComponent>())
        {
            UIButtonComponent copiedButton = entity.GetComponent<UIButtonComponent>();
            copiedButton.IsPointerInside = false;
            copiedButton.IsPressed = false;
            copiedButton.WasClickedThisFrame = false;
            newEntity.AddComponent<UIButtonComponent>(copiedButton);
        }
        if (entity.HasComponent<SoundPlayerComponent>())
        {
            SoundPlayerComponent copiedSoundPlayer = entity.GetComponent<SoundPlayerComponent>();
            copiedSoundPlayer.RuntimeVoiceHandle = AudioEngine::InvalidVoiceHandle;
            copiedSoundPlayer.RuntimePaused = false;
            newEntity.AddComponent<SoundPlayerComponent>(copiedSoundPlayer);
        }

        return newEntity;
    }

    Entity Scene::FindEntityByName(const std::string &name)
    {
        auto view = m_Registry.view<TagComponent>();
        for (auto entity: view)
        {
            const TagComponent &tc = view.get<TagComponent>(entity);
            if (tc.Tag == name)
                return Entity{entity, this};
        }
        return {};
    }

    Entity Scene::GetEntityByUUID(UUID uuid)
    {
        if (m_EntityMap.find(uuid) != m_EntityMap.end())
            return {m_EntityMap.at(uuid), this};

        return {};
    }

    Entity Scene::GetEntityByUUID(UUID uuid) const
    {
        return const_cast<Scene*>(this)->GetEntityByUUID(uuid);
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        if (m_ViewportWidth == width && m_ViewportHeight == height)
            return;

        m_ViewportWidth = width;
        m_ViewportHeight = height;

        auto view = m_Registry.view<CameraComponent>();
        for (auto entity: view)
        {
            auto &cameraComponent = view.get<CameraComponent>(entity);
            if (!cameraComponent.FixedAspectRatio)
                cameraComponent.Camera.SetViewportSize(width, height);
        }
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_Registry.view<TransformComponent, CameraComponent>();
        for (auto entity: view)
        {
            const auto &cameraComponent = view.get<CameraComponent>(entity);
            if (cameraComponent.Primary)
            {
                return Entity{entity, this};
            }
        }
        return {};
    }

    template<typename T>
    void Scene::OnComponentAdded(Entity emtity, T &component)
    {
    }
    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent &component)
    {
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
        {
            component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
        }
    }

    template<>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent &component)
    {
        (void)entity;
        (void)component;
    }

    template<>
    void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent &component)
    {
    }

    template<>
    void Scene::OnComponentAdded<SpriteAnimationComponent>(Entity entity, SpriteAnimationComponent &component)
    {
        if (!entity.HasComponent<SpriteRendererComponent>())
            HIMII_CORE_WARNING("Entity '{0}' has Sprite Animation but no Sprite Renderer.",
                            entity.GetName());
    }

    template<>
    void Scene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent &component)
    {
        (void)entity;
        NormalizeMeshComponentMaterialSlots(component);
    }

    template<>
    void Scene::OnComponentAdded<TilemapComponent>(Entity entity, TilemapComponent &component)
    {
    }
    template<>
    void Scene::OnComponentAdded<TilemapCollider2DComponent>(Entity entity,
                                                             TilemapCollider2DComponent& component)
    {
    }
    template<>
    void Scene::OnComponentAdded<ParticleEmitterComponent>(Entity entity, ParticleEmitterComponent &component)
    {
    }
    template<>
    void Scene::OnComponentAdded<RectTransformComponent>(
            Entity entity, RectTransformComponent &component)
    {
        (void)entity;
        (void)component;
    }
    template<>
    void Scene::OnComponentAdded<UIImageComponent>(Entity entity, UIImageComponent &component)
    {
    }
    template<>
    void Scene::OnComponentAdded<UITextComponent>(Entity entity, UITextComponent &component)
    {
        if (!component.FontAsset)
            component.FontAsset = Font::GetDefault();
    }
    template<>
    void Scene::OnComponentAdded<UIButtonComponent>(Entity entity, UIButtonComponent &component)
    {
        (void)entity;
        (void)component;
    }
    template<>
    void Scene::OnComponentAdded<SoundPlayerComponent>(Entity entity, SoundPlayerComponent &component)
    {
        (void)entity;
        component.RuntimeVoiceHandle = AudioEngine::InvalidVoiceHandle;
        component.RuntimePaused = false;
        SoundPlayerUtility::ResolveSoundAsset(component);
    }
    template<>
    void Scene::OnComponentAdded<CanvasComponent>(Entity entity, CanvasComponent &component)
    {
        (void)component;
        SyncCanvasReferenceResolutionToTransform(entity);
    }
    template<>
    void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent &component)
    {
        (void)entity;
        (void)component;
    }

} // namespace Himii
