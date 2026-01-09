#include "Scene.h"
#include "Entity.h"
#include "Hepch.h"

#include "Components.h"
#include "Himii/Asset/AssetManager.h"
#include "Himii/Project/Project.h"
#include "Himii/Renderer/Renderer2D.h"
#include "Himii/Scene/SpriteAnimation.h"
#include "Himii/Scripting/ScriptEngine.h"
#include "ScriptableEntity.h"

#include <glm/glm.hpp>

#include "Entity.h"

namespace Himii
{
    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
        if (b2World_IsValid(m_Box2DWorld))
        {
            b2DestroyWorld(m_Box2DWorld);
        }
    }

    static void *ToPtr(b2BodyId id)
    {
        return reinterpret_cast<void *>(*reinterpret_cast<uintptr_t *>(&id));
    }
    static b2BodyId ToBodyId(void *ptr)
    {
        uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
        return *reinterpret_cast<b2BodyId *>(&val);
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

    void Scene::DestroyEntity(entt::entity e)
    {
        if (!m_Registry.valid(e))
            return;

        if (auto *pid = m_Registry.try_get<IDComponent>(e))
        {
            auto it = m_EntityMap.find(pid->ID);
            if (it != m_EntityMap.end())
                m_EntityMap.erase(it);
        }
        // 脚本析构
        if (NativeScriptComponent *nsc = m_Registry.try_get<NativeScriptComponent>(e))
        {
            if (nsc->Instance)
            {
                nsc->Instance->OnDestroy();
            }
            if (nsc->DestroyScript)
            {
                nsc->DestroyScript(nsc);
            }
        }
        m_Registry.destroy(e);
    }

    void Scene::OnRuntimeStart()
    {
        ScriptEngine::OnRuntimeStart(this);
        OnPhysics2DStart();
        // 3. 实例化所有拥有 ScriptComponent 的实体
        {
            auto view = m_Registry.view<ScriptComponent>();
            for (auto e: view)
            {
                Entity entity = {e, this};
                ScriptEngine::OnCreateEntity(entity); // <--- 新增这行调用
            }
        }
    }

    void Scene::OnSimulationStart()
    {
        OnPhysics2DStart();
    }

    void Scene::OnSimulationStop()
    {
        OnPhysics2DStop();
    }

    void Scene::OnRuntimeStop()
    {
        OnPhysics2DStop();
        ScriptEngine::OnRuntimeStop();
    }

    void Scene::OnUpdateEditor(Timestep ts, EditorCamera &camera)
    {
        RenderScene(camera);
    }

    void Scene::OnUpdateRuntime(Timestep ts)
    {
        {
            auto view = m_Registry.view<ScriptComponent>();
            for (auto e: view)
            {
                Entity entity = {e, this};
                ScriptEngine::OnUpdateScript(entity, ts);
            }

            m_Registry.view<NativeScriptComponent>().each(
                    [=](auto entity, auto &nsc)
                    {
                        // TODO: Move to Scene::OnScenePlay
                        if (!nsc.Instance)
                        {
                            nsc.Instance = nsc.InstantiateScript();
                            nsc.Instance->m_Entity = Entity{entity, this};
                            nsc.Instance->OnCreate();
                        }

                        nsc.Instance->OnUpdate(ts);
                    });
        }

        // Animation Update
        {
            auto view = m_Registry.group<SpriteAnimationComponent, SpriteRendererComponent>();

            // 获取 AssetManager
            auto assetManager = Project::GetAssetManager();

            for (auto e: view)
            {
                auto [animComponent, spriteComponent] = view.get<SpriteAnimationComponent, SpriteRendererComponent>(e);

                // 确保有有效的动画资产句柄
                if (animComponent.AnimationHandle != 0 && animComponent.Playing)
                {
                    // 从 AssetManager 获取动画数据
                    if (assetManager->IsAssetHandleValid(animComponent.AnimationHandle))
                    {
                        Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                                assetManager->GetAsset(animComponent.AnimationHandle));

                        if (animation && animation->GetFrameCount() > 0)
                        {
                            // 计时器逻辑
                            animComponent.Timer += ts;
                            float frameDuration = 1.0f / animComponent.FrameRate;

                            if (animComponent.Timer >= frameDuration)
                            {
                                animComponent.Timer -= frameDuration;
                                animComponent.CurrentFrame =
                                        (animComponent.CurrentFrame + 1) % animation->GetFrameCount();
                            }

                            // 获取当前帧的纹理句柄
                            AssetHandle textureHandle = animation->GetFrame(animComponent.CurrentFrame);

                            // 将纹理应用到 SpriteRenderer
                            if (assetManager->IsAssetHandleValid(textureHandle))
                            {
                                Ref<Texture2D> texture =
                                        std::static_pointer_cast<Texture2D>(assetManager->GetAsset(textureHandle));
                                spriteComponent.Texture = texture;
                            }
                        }
                    }
                }
            }
        }

        // Box2D 物理更新
        {
            const int32_t subStepCount = 2;
            b2World_Step(m_Box2DWorld, ts, subStepCount);

            auto view = m_Registry.view<Rigidbody2DComponent>();
            for (auto e: view)
            {
                Entity entity = {e, this};
                auto &transform = entity.GetComponent<TransformComponent>();
                auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();

                if (rb2d.RuntimeBody)
                {
                    b2BodyId bodyId = ToBodyId(rb2d.RuntimeBody);
                    if (b2Body_IsValid(bodyId))
                    {
                        b2Vec2 position = b2Body_GetPosition(bodyId);
                        transform.Position.x = position.x;
                        transform.Position.y = position.y;

                        b2Rot rotation = b2Body_GetRotation(bodyId);
                        transform.Rotation.z = b2Rot_GetAngle(rotation);
                    }
                }
            }
        }

        Camera *mainCamera = nullptr;
        glm::mat4 cameraTransform{1.0f};
        {
            auto view = m_Registry.view<TransformComponent, CameraComponent>();
            view.each(
                    [&](entt::entity entity, TransformComponent &transform, CameraComponent &camera)
                    {
                        if (camera.Primary)
                        {
                            mainCamera = &camera.Camera;
                            cameraTransform = transform.GetTransform();
                        }
                    });
        }

        if (mainCamera)
        {
            Renderer2D::BeginScene(*mainCamera, cameraTransform);
            {
                auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
                view.each([&](entt::entity entity, TransformComponent &transform, SpriteRendererComponent &sprite)
                          { Himii::Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity); });
            }
            {
                auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
                view.each(
                        [&](entt::entity entity, TransformComponent &transform, CircleRendererComponent &circle) {
                            Himii::Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness,
                                                          circle.Fade, (int)entity);
                        });
            }
            Renderer2D::EndScene();
        }
    }

    void Scene::OnUpdateSimulation(Timestep ts, EditorCamera &camera)
    {
        // Box2D 物理更新
        {
            const int32_t subStepCount = 2;
            b2World_Step(m_Box2DWorld, ts, subStepCount);

            auto view = m_Registry.view<Rigidbody2DComponent>();
            for (auto e: view)
            {
                Entity entity = {e, this};
                auto &transform = entity.GetComponent<TransformComponent>();
                auto &rb2d = entity.GetComponent<Rigidbody2DComponent>();

                if (rb2d.RuntimeBody)
                {
                    b2BodyId bodyId = ToBodyId(rb2d.RuntimeBody);
                    if (b2Body_IsValid(bodyId))
                    {
                        b2Vec2 position = b2Body_GetPosition(bodyId);
                        transform.Position.x = position.x;
                        transform.Position.y = position.y;

                        b2Rot rotation = b2Body_GetRotation(bodyId);
                        transform.Rotation.z = b2Rot_GetAngle(rotation);
                    }
                }
            }
        }

        RenderScene(camera);
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
            auto &component = src.get<Component>(e);
            dst.emplace_or_replace<Component>(dstEnttID, component);
        }
    }

    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref<Scene> newScene = CreateRef<Scene>();

        newScene->m_ViewportWidth = other->m_ViewportWidth;
        newScene->m_ViewportHeight = other->m_ViewportHeight;

        auto &srcSceneRegistry = other->m_Registry;
        auto &dstSceneRegistry = newScene->m_Registry;
        std::unordered_map<UUID, entt::entity> enttMap;

        // Create entities in new scene
        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e: idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto &name = srcSceneRegistry.get<TagComponent>(e).Tag;
            Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
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

        return newScene;
    }

    Entity Scene::DuplicateEntity(Entity entity)
    {
        std::string name = entity.GetName();
        Entity newEntity = CreateEntity(name);

        if (entity.HasComponent<TransformComponent>())
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
        // TODO(Yan): Maybe should be assert
        if (m_EntityMap.find(uuid) != m_EntityMap.end())
            return {m_EntityMap.at(uuid), this};

        return {};
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
        auto view = m_Registry.view<CameraComponent>();
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

    void Scene::OnPhysics2DStart()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{0.0f, -9.8f};
        m_Box2DWorld = b2CreateWorld(&worldDef);

        auto view = m_Registry.view<Rigidbody2DComponent>();
        for (auto e: view)
        {
            Entity entity = {e, this};
            auto &transform = entity.GetComponent<TransformComponent>();
            auto &rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();

            b2BodyDef bodyDef = b2DefaultBodyDef();

            switch (rigidbody2D.Type)
            {
                case Rigidbody2DComponent::BodyType::Static:
                    bodyDef.type = b2BodyType::b2_staticBody;
                    break;
                case Rigidbody2DComponent::BodyType::Dynamic:
                    bodyDef.type = b2BodyType::b2_dynamicBody;
                    break;
                case Rigidbody2DComponent::BodyType::Kinematic:
                    bodyDef.type = b2BodyType::b2_kinematicBody;
                    break;
            }

            bodyDef.position = {transform.Position.x, transform.Position.y};
            bodyDef.rotation = b2MakeRot(transform.Rotation.z);
            bodyDef.fixedRotation = rigidbody2D.FixedRotation;
            bodyDef.userData = (void *)(uintptr_t)(uint32_t)entity; // 存储 Entity ID

            b2BodyId bodyId = b2CreateBody(m_Box2DWorld, &bodyDef);
            rigidbody2D.RuntimeBody = ToPtr(bodyId);

            // 3. 添加碰撞体
            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto &bc2d = entity.GetComponent<BoxCollider2DComponent>();

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = bc2d.Density;
                shapeDef.material.friction = bc2d.Friction;
                shapeDef.material.restitution = bc2d.Restitution;
                shapeDef.material.rollingResistance = bc2d.RestitutionThreshold;

                // Box2D v3 b2MakeBox 参数是半宽/半高
                float hx = bc2d.Size.x * transform.Scale.x * 0.5f;
                float hy = bc2d.Size.y * transform.Scale.y * 0.5f;

                b2Polygon polygon = b2MakeBox(hx, hy);
                // 应用 Offset
                polygon.centroid = {bc2d.Offset.x, bc2d.Offset.y};

                b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
            }

            if (entity.HasComponent<CircleCollider2DComponent>())
            {
                auto &cc2d = entity.GetComponent<CircleCollider2DComponent>();

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = cc2d.Density;
                shapeDef.material.friction = cc2d.Friction;
                shapeDef.material.restitution = cc2d.Restitution;
                shapeDef.material.rollingResistance = cc2d.RestitutionThreshold;

                b2Circle circle;
                // 应用 Offset
                circle.center = {cc2d.Offset.x * transform.Scale.x, cc2d.Offset.y * transform.Scale.y};
                float maxScale = std::max(transform.Scale.x, transform.Scale.y);
                circle.radius = cc2d.Radius * maxScale;

                b2CreateCircleShape(bodyId, &shapeDef, &circle);
            }
        }
    }

    void Scene::OnPhysics2DStop()
    {
        if (b2World_IsValid(m_Box2DWorld))
        {
            b2DestroyWorld(m_Box2DWorld);
            m_Box2DWorld = {0}; // Reset ID
        }
    }

    void Scene::RenderScene(EditorCamera &camera)
    {
        Renderer2D::BeginScene(camera);

        // Draw Sprites
        {
            auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
            view.each([&](entt::entity entity, TransformComponent &transform, SpriteRendererComponent &sprite)
                      { Himii::Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity); });
        }

        // Draw Circles
        {
            auto view = m_Registry.view<TransformComponent, CircleRendererComponent>();
            view.each(
                    [&](entt::entity entity, TransformComponent &transform, CircleRendererComponent &circle) {
                        Himii::Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness,
                                                      circle.Fade, (int)entity);
                    });
        }

        Renderer2D::EndScene();
    }

    //
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
    }

    template<>
    void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent &component)
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
    }

} // namespace Himii
