#include "Hepch.h"
#include "Module/Physics/Physics2DWorld.h"

#include "World/Scene/Entity.h"
#include "World/Scene/Components.h"
#include "World/Scene/SceneInternal.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Tilemap/TilemapColliderBuilder.h"
#include "Module/Script/ScriptEngine.h"
#include "EngineCore/Core/Log.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Himii
{
    namespace
    {
        b2Filter BuildColliderShapeFilter(int layerIndex)
        {
            if (Project::GetActive())
                return Project::GetConfig().Physics2DLayers.BuildShapeFilter(layerIndex);

            b2Filter filter = b2DefaultFilter();
            filter.categoryBits = 1u;
            filter.maskBits = 0xFFFFFFFFu;
            return filter;
        }

        void AttachBoxColliderToBody(b2BodyId bodyId,
                                     const BoxCollider2DComponent &boxCollider,
                                     const glm::vec3 &worldScale)
        {
            if (!b2Body_IsValid(bodyId))
                return;

            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.enableContactEvents = true;
            shapeDef.isSensor = boxCollider.IsTrigger;
            shapeDef.filter = BuildColliderShapeFilter(boxCollider.Layer);
            shapeDef.density = boxCollider.Density;
            shapeDef.material.friction = boxCollider.Friction;
            shapeDef.material.restitution = boxCollider.Restitution;
            shapeDef.material.rollingResistance = boxCollider.RestitutionThreshold;

            const float absoluteScaleX = std::abs(worldScale.x);
            const float absoluteScaleY = std::abs(worldScale.y);
            const float halfWidth = boxCollider.Size.x * absoluteScaleX * 0.5f;
            const float halfHeight = boxCollider.Size.y * absoluteScaleY * 0.5f;
            const b2Polygon polygon = b2MakeOffsetBox(
                    halfWidth, halfHeight,
                    {boxCollider.Offset.x * worldScale.x, boxCollider.Offset.y * worldScale.y},
                    b2MakeRot(0.0f));

            b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
        }

        void AttachCircleColliderToBody(b2BodyId bodyId,
                                        const CircleCollider2DComponent &circleCollider,
                                        const glm::vec3 &worldScale)
        {
            if (!b2Body_IsValid(bodyId))
                return;

            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.enableContactEvents = true;
            shapeDef.isSensor = circleCollider.IsTrigger;
            shapeDef.filter = BuildColliderShapeFilter(circleCollider.Layer);
            shapeDef.density = circleCollider.Density;
            shapeDef.material.friction = circleCollider.Friction;
            shapeDef.material.restitution = circleCollider.Restitution;
            shapeDef.material.rollingResistance = circleCollider.RestitutionThreshold;

            b2Circle circle;
            circle.center = {
                    circleCollider.Offset.x * worldScale.x,
                    circleCollider.Offset.y * worldScale.y};
            const float absoluteScaleX = std::abs(worldScale.x);
            const float absoluteScaleY = std::abs(worldScale.y);
            const float maxScale = std::max(absoluteScaleX, absoluteScaleY);
            circle.radius = circleCollider.Radius * maxScale;

            b2CreateCircleShape(bodyId, &shapeDef, &circle);
        }

        b2BodyId CreateStaticPhysicsBody(b2WorldId world, Scene *scene, Entity entity)
        {
            const glm::vec3 worldTranslation = scene->GetEntityWorldTranslation(entity);
            const glm::vec3 worldRotation = scene->GetEntityWorldRotation(entity);

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_staticBody;
            bodyDef.position = {worldTranslation.x, worldTranslation.y};
            bodyDef.rotation = b2MakeRot(worldRotation.z);
            bodyDef.userData = (void *)(uintptr_t)(uint32_t)(entt::entity)entity;
            return b2CreateBody(world, &bodyDef);
        }

        float RayCastCallback(b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void *context)
        {
            auto *callbackContext = (std::pair<Scene *, Scene::RaycastHit2D *> *)context;
            Scene *scene = callbackContext->first;
            Scene::RaycastHit2D *hit = callbackContext->second;

            hit->Hit = true;
            hit->Point = {point.x, point.y};
            hit->Normal = {normal.x, normal.y};
            hit->Distance = fraction;

            b2BodyId bodyId = b2Shape_GetBody(shapeId);
            void *userData = b2Body_GetUserData(bodyId);
            uint32_t entityHandle = (uint32_t)(uintptr_t)userData;

            entt::entity entityIdentifier = (entt::entity)entityHandle;
            if (scene->Registry().valid(entityIdentifier))
            {
                hit->EntityID = scene->Registry().get<IDComponent>(entityIdentifier).ID;
            }

            return fraction;
        }
    }

    Physics2DWorld::Physics2DWorld(Scene *scene) : m_Scene(scene)
    {
    }

    Physics2DWorld::~Physics2DWorld()
    {
        Stop();
    }

    void Physics2DWorld::Start()
    {
        if (!m_Scene)
            return;

        Stop();

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{0.0f, -9.8f};
        m_Box2DWorld = b2CreateWorld(&worldDef);

        auto &registry = m_Scene->Registry();
        auto view = registry.view<Rigidbody2DComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene};
            auto &rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
            const glm::vec3 worldTranslation = m_Scene->GetEntityWorldTranslation(entity);
            const glm::vec3 worldRotation = m_Scene->GetEntityWorldRotation(entity);
            const glm::vec3 worldScale = m_Scene->GetEntityWorldScale(entity);

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

            bodyDef.position = {worldTranslation.x, worldTranslation.y};
            bodyDef.rotation = b2MakeRot(worldRotation.z);
            bodyDef.fixedRotation = rigidbody2D.FixedRotation;
            bodyDef.userData = (void *)(uintptr_t)(uint32_t)entity;

            b2BodyId bodyId = b2CreateBody(m_Box2DWorld, &bodyDef);
            rigidbody2D.RuntimeBody = SceneInternal::BodyIdToPointer(bodyId);

            if (entity.HasComponent<BoxCollider2DComponent>())
                AttachBoxColliderToBody(bodyId, entity.GetComponent<BoxCollider2DComponent>(), worldScale);

            if (entity.HasComponent<CircleCollider2DComponent>())
                AttachCircleColliderToBody(bodyId, entity.GetComponent<CircleCollider2DComponent>(), worldScale);
        }

        {
            auto colliderOnlyView = registry.view<TransformComponent>();
            for (auto entityHandle : colliderOnlyView)
            {
                if (registry.any_of<Rigidbody2DComponent>(entityHandle))
                    continue;

                const bool hasBoxCollider = registry.all_of<BoxCollider2DComponent>(entityHandle);
                const bool hasCircleCollider = registry.all_of<CircleCollider2DComponent>(entityHandle);
                if (!hasBoxCollider && !hasCircleCollider)
                    continue;

                Entity entity = {entityHandle, m_Scene};
                const glm::vec3 worldScale = m_Scene->GetEntityWorldScale(entity);
                const b2BodyId staticBodyId = CreateStaticPhysicsBody(m_Box2DWorld, m_Scene, entity);

                if (hasBoxCollider)
                    AttachBoxColliderToBody(
                            staticBodyId, entity.GetComponent<BoxCollider2DComponent>(), worldScale);
                if (hasCircleCollider)
                    AttachCircleColliderToBody(
                            staticBodyId, entity.GetComponent<CircleCollider2DComponent>(), worldScale);
            }
        }

        auto tilemapColliderView =
                registry.view<TransformComponent, TilemapComponent, TilemapCollider2DComponent>();
        for (auto entityHandle : tilemapColliderView)
        {
            Entity entity = {entityHandle, m_Scene};
            auto &transform = entity.GetComponent<TransformComponent>();
            auto &tilemap = entity.GetComponent<TilemapComponent>();
            auto &tilemapCollider = entity.GetComponent<TilemapCollider2DComponent>();

            if (!tilemapCollider.Enabled || tilemap.TileMapHandle == 0)
                continue;

            auto assetManager = ResourceSystem::GetAssetManager();
            if (!assetManager)
                continue;

            Ref<TileMapData> mapData = std::static_pointer_cast<TileMapData>(
                    assetManager->GetAsset(tilemap.TileMapHandle));
            if (!mapData)
                continue;

            if (mapData->GetCellSize() <= 0.0f)
                continue;

            Ref<TileSet> tileSet;
            if (mapData->GetTileSetHandle() != 0)
            {
                tileSet = std::static_pointer_cast<TileSet>(
                        assetManager->GetAsset(mapData->GetTileSetHandle()));
            }

            if (!tileSet)
            {
                HIMII_CORE_WARNING(
                        "TilemapCollider2D: entity '{0}' has no TileSet; colliders were not created.",
                        entity.GetName());
                continue;
            }

            void *bodyUserData = (void *)(uintptr_t)(uint32_t)entity;
            const TilemapColliderBuildReport report = TilemapColliderBuilder::CreateColliderShapes(
                    m_Box2DWorld,
                    transform,
                    *mapData,
                    *tileSet,
                    bodyUserData,
                    tilemapCollider.MergeAdjacentCells);

            TilemapColliderBuilder::LogBuildReport(entity.GetName(), report);
        }
    }

    void Physics2DWorld::Stop()
    {
        if (b2World_IsValid(m_Box2DWorld))
        {
            b2DestroyWorld(m_Box2DWorld);
            m_Box2DWorld = {};
        }
    }

    void Physics2DWorld::Step(Timestep timestep)
    {
        if (!m_Scene || !b2World_IsValid(m_Box2DWorld))
            return;

        const int32_t subStepCount = 2;
        b2World_Step(m_Box2DWorld, timestep, subStepCount);
        ProcessContacts();

        auto view = m_Scene->Registry().view<Rigidbody2DComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene};
            auto &rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();

            if (rigidbody2D.RuntimeBody)
            {
                b2BodyId bodyId = SceneInternal::PointerToBodyId(rigidbody2D.RuntimeBody);
                if (b2Body_IsValid(bodyId))
                {
                    b2Vec2 position = b2Body_GetPosition(bodyId);
                    b2Rot rotation = b2Body_GetRotation(bodyId);
                    m_Scene->ApplyPhysicsWorldTransform(
                            entity, {position.x, position.y}, b2Rot_GetAngle(rotation));
                }
            }
        }
    }

    Scene::RaycastHit2D Physics2DWorld::Raycast2D(glm::vec2 start, glm::vec2 end)
    {
        Scene::RaycastHit2D hit;
        hit.Hit = false;

        if (!m_Scene || !b2World_IsValid(m_Box2DWorld))
            return hit;

        b2Vec2 origin = {start.x, start.y};
        b2Vec2 translation = {end.x - start.x, end.y - start.y};
        b2QueryFilter filter = b2DefaultQueryFilter();

        std::pair<Scene *, Scene::RaycastHit2D *> context = {m_Scene, &hit};
        b2World_CastRay(m_Box2DWorld, origin, translation, filter, RayCastCallback, &context);

        if (hit.Hit)
        {
            float length = glm::length(end - start);
            hit.Distance *= length;
        }

        return hit;
    }

    void Physics2DWorld::SyncEntityTransform(Entity entity)
    {
        if (!m_Scene || !b2World_IsValid(m_Box2DWorld))
            return;

        if (!entity || !entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto &rigidbody2D = entity.GetComponent<Rigidbody2DComponent>();
        if (!rigidbody2D.RuntimeBody)
            return;

        b2BodyId bodyId = SceneInternal::PointerToBodyId(rigidbody2D.RuntimeBody);
        if (!b2Body_IsValid(bodyId))
            return;

        const glm::vec3 worldTranslation = m_Scene->GetEntityWorldTranslation(entity);
        const glm::vec3 worldRotation = m_Scene->GetEntityWorldRotation(entity);
        b2Body_SetTransform(
                bodyId,
                {worldTranslation.x, worldTranslation.y},
                b2MakeRot(worldRotation.z));
    }

    Entity Physics2DWorld::GetEntityFromShape(b2ShapeId shapeId)
    {
        if (!m_Scene || !b2Shape_IsValid(shapeId))
            return {};

        b2BodyId bodyId = b2Shape_GetBody(shapeId);
        if (!b2Body_IsValid(bodyId))
            return {};

        void *userData = b2Body_GetUserData(bodyId);
        if (!userData)
            return {};

        entt::entity handle =
                static_cast<entt::entity>(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(userData)));
        if (!m_Scene->Registry().valid(handle))
            return {};

        return Entity{handle, m_Scene};
    }

    void Physics2DWorld::ProcessContacts()
    {
        if (!m_Scene || !b2World_IsValid(m_Box2DWorld))
            return;

        b2ContactEvents events = b2World_GetContactEvents(m_Box2DWorld);

        for (int index = 0; index < events.beginCount; ++index)
        {
            const b2ContactBeginTouchEvent &event = events.beginEvents[index];
            Entity entityA = GetEntityFromShape(event.shapeIdA);
            Entity entityB = GetEntityFromShape(event.shapeIdB);
            if (entityA && entityB)
            {
                const bool isTriggerContact =
                        b2Shape_IsSensor(event.shapeIdA) || b2Shape_IsSensor(event.shapeIdB);

                auto buildCollisionInterop = [&](Entity self, Entity other) -> Collision2DInterop
                {
                    Collision2DInterop collisionInterop;
                    collisionInterop.OtherEntityID = other.GetUUID();

                    const Entity shapeOwnerA = GetEntityFromShape(event.shapeIdA);
                    const b2Vec2 &manifoldNormal = event.manifold.normal;
                    if (shapeOwnerA == self)
                        collisionInterop.Normal = glm::vec2{-manifoldNormal.x, -manifoldNormal.y};
                    else
                        collisionInterop.Normal = glm::vec2{manifoldNormal.x, manifoldNormal.y};

                    collisionInterop.Point = glm::vec2{0.0f, 0.0f};
                    if (event.manifold.pointCount > 0)
                    {
                        const b2Vec2 &contactPoint = event.manifold.points[0].point;
                        collisionInterop.Point = glm::vec2{contactPoint.x, contactPoint.y};
                    }

                    return collisionInterop;
                };

                if (isTriggerContact)
                {
                    ScriptEngine::OnTriggerEnter2D(entityA, entityB, buildCollisionInterop(entityA, entityB));
                    ScriptEngine::OnTriggerEnter2D(entityB, entityA, buildCollisionInterop(entityB, entityA));
                }
                else
                {
                    ScriptEngine::OnCollisionEnter2D(entityA, entityB, buildCollisionInterop(entityA, entityB));
                    ScriptEngine::OnCollisionEnter2D(entityB, entityA, buildCollisionInterop(entityB, entityA));
                }
            }
        }

        for (int index = 0; index < events.endCount; ++index)
        {
            const b2ContactEndTouchEvent &event = events.endEvents[index];
            if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB))
                continue;

            Entity entityA = GetEntityFromShape(event.shapeIdA);
            Entity entityB = GetEntityFromShape(event.shapeIdB);
            if (entityA && entityB)
            {
                const bool isTriggerContact =
                        b2Shape_IsSensor(event.shapeIdA) || b2Shape_IsSensor(event.shapeIdB);

                if (isTriggerContact)
                {
                    ScriptEngine::OnTriggerExit2D(entityA, entityB);
                    ScriptEngine::OnTriggerExit2D(entityB, entityA);
                }
                else
                {
                    ScriptEngine::OnCollisionExit2D(entityA, entityB);
                    ScriptEngine::OnCollisionExit2D(entityB, entityA);
                }
            }
        }
    }
}
