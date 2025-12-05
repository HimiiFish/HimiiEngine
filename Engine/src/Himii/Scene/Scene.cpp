#include "Scene.h"
#include "Entity.h"
#include "hepch.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "Himii/Renderer/Renderer2D.h"

#include <glm/glm.hpp>

#include "Entity.h"

namespace Himii
{
    static void* ToPtr(b2BodyId id) {
        return reinterpret_cast<void*>(*reinterpret_cast<uintptr_t*>(&id));
    }
    static b2BodyId ToBodyId(void* ptr) {
        uintptr_t val = reinterpret_cast<uintptr_t>(ptr);
        return *reinterpret_cast<b2BodyId*>(&val);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
    {
        Entity entity(m_Registry.create(), this);

        entity.AddComponent<IDComponent>(uuid);
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.name = name.empty() ? "Entity" : name;

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
            auto it = m_EntityMap.find(pid->id);
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
        b2WorldDef worldDef=b2DefaultWorldDef();
        worldDef.gravity= b2Vec2{0.0f,-9.8f};
        m_Box2DWorld=b2CreateWorld(&worldDef);

        auto view = m_Registry.view<Rigidbody2DComponent>();
        for (auto e : view)
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

            bodyDef.position = { transform.Position.x, transform.Position.y };
            bodyDef.rotation = b2MakeRot(transform.Rotation.z);
            bodyDef.fixedRotation = rigidbody2D.FixedRotation;
            bodyDef.userData = (void*)(uintptr_t)(uint32_t)entity; // 存储 Entity ID

            b2BodyId bodyId = b2CreateBody(m_Box2DWorld, &bodyDef);
            rigidbody2D.RuntimeBody = ToPtr(bodyId);

            // 3. 添加碰撞体
            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

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
                polygon.centroid = { bc2d.Offset.x, bc2d.Offset.y };

                b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
            }
        }
    }

    void Scene::OnRuntimeStop()
    {
        if (b2World_IsValid(m_Box2DWorld))
        {
            b2DestroyWorld(m_Box2DWorld);
            m_Box2DWorld = {0}; // Reset ID
        }
    }

    void Scene::OnUpdateEditor(Timestep ts, EditorCamera &camera)
    {
        Renderer2D::BeginScene(camera);
        auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
        for (auto entity: group)
        {
            auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
            Himii::Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
        }
        Renderer2D::EndScene();
    }

    void Scene::OnUpdateRuntime(Timestep ts)
    {
        {
            m_Registry.view<NativeScriptComponent>().each(
                    [=](auto entity, auto &nsc)
                    {
                        // 如果没有实例化，尝试实例化
                        if (!nsc.Instance)
                        {
                            nsc.Instance = nsc.InstantiateScript();
                            nsc.Instance->m_Entity = Entity{entity, this};
                            nsc.Instance->OnCreate();
                        }
                        // 更新
                        nsc.Instance->OnUpdate(ts);
                    });
        }

        // Box2D 物理更新
        {
            const int32_t subStepCount = 4;
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
                        if (camera.primary)
                        {
                            mainCamera = &camera.camera;
                            cameraTransform = transform.GetTransform();
                        }
                    });
        }

        if (mainCamera)
        {
            Renderer2D::BeginScene(*mainCamera, cameraTransform);
            auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
            for (auto entity: group)
            {
                auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
                Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
            }
            Renderer2D::EndScene();
        }

        // 如果没有可用摄像机，保留上层（例如 CubeLayer）外部调用 BeginScene 的能力

        // 3D MeshRenderer - Skybox pass first (depth: LEQUAL, no depth write)
        /*{
            auto skyView = m_Registry.view<Himii::Transform, Himii::MeshRenderer, Himii::SkyboxTag>();
        if (skyView.begin() != skyView.end()) {
                if (viewProj != glm::mat4(1.0f)) Himii::Renderer::BeginScene(viewProj);
                glDepthFunc(GL_LEQUAL);
                glDepthMask(GL_FALSE);
                for (auto e : skyView) {
                    auto &tr = skyView.get<Himii::Transform>(e);
                    const auto &mr = skyView.get<Himii::MeshRenderer>(e);
                    if (mr.vertexArray && mr.shader) {
                        if (mr.texture) mr.texture->Bind(0);
                        Himii::Renderer::Submit(mr.shader, mr.vertexArray, tr.GetTransform());
                    }
                }
                glDepthMask(GL_TRUE);
                glDepthFunc(GL_LESS);
                if (viewProj != glm::mat4(1.0f)) Himii::Renderer::EndScene();
            }
        }*/

        // 3D MeshRenderer - regular pass (exclude skybox)
        /* {
             auto meshView = m_Registry.view<Himii::Transform, Himii::MeshRenderer>(entt::exclude<Himii::SkyboxTag>);
             if (meshView.begin() != meshView.end() && viewProj != glm::mat4(1.0f))
         Himii::Renderer::BeginScene(viewProj); for (auto e : meshView) { auto &tr = meshView.get<Himii::Transform>(e);
                 const auto &mr = meshView.get<Himii::MeshRenderer>(e);
                 if (mr.vertexArray && mr.shader) {
                     if (mr.texture) mr.texture->Bind(0);
                     Himii::Renderer::Submit(mr.shader, mr.vertexArray, tr.GetTransform());
                 }
             }
             if (meshView.begin() != meshView.end() && viewProj != glm::mat4(1.0f)) Himii::Renderer::EndScene();
         }*/

        // 2D SpriteRenderer 实体：Renderer2D 批渲染


        // 原生脚本更新
        
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
            if (!cameraComponent.fixedAspectRatio)
                cameraComponent.camera.SetViewportSize(width, height);
        }
    }

    void Scene::Clear()
    {
        // 收集后再销毁，避免遍历时失效
        std::vector<entt::entity> toDelete;
        auto view = m_Registry.view<TransformComponent>();
        for (auto e: view)
            toDelete.push_back(e);
        for (auto e: toDelete)
            DestroyEntity(e);
    }

    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_Registry.view<CameraComponent>();
        for (auto entity: view)
        {
            const auto &cameraComponent = view.get<CameraComponent>(entity);
            if (cameraComponent.primary)
            {
                return Entity{entity, this};
            }
        }
        return {};
    }

    //
    template<typename T>
    void Scene::OnComponentAdded(Entity emtity, T &component)
    {
        static_cast(sizeof(T) == 0);
    }
    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent &component)
    {
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
        {
            component.camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
        }
    }

} // namespace Himii
