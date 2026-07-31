#include "Scene.h"
#include "Entity.h"
#include "Hepch.h"
#include "Components.h"
#include "World/Scene/SceneInternal.h"
#include "EngineCore/Math/Math.h"
#include "box2d/box2d.h"

#include <algorithm>
#include <vector>

namespace Himii
{
    bool Scene::EntitiesShareTransformDomain(Entity left, Entity right) const
    {
        if (!left || !right)
            return false;

        const bool leftIsUserInterface = left.HasComponent<RectTransformComponent>();
        const bool rightIsUserInterface = right.HasComponent<RectTransformComponent>();
        return leftIsUserInterface == rightIsUserInterface;
    }

    void Scene::RebuildHierarchyCache()
    {
        m_ChildrenCache.clear();

        auto relationshipView = m_Registry.view<RelationshipComponent>();
        for (auto entityHandle : relationshipView)
        {
            Entity entity{entityHandle, this};
            const UUID parentIdentifier = entity.GetComponent<RelationshipComponent>().Parent;
            if (parentIdentifier == 0)
                continue;

            m_ChildrenCache[parentIdentifier].push_back(entity.GetUUID());
        }

        for (auto& [parentIdentifier, children] : m_ChildrenCache)
        {
            (void)parentIdentifier;
            std::sort(
                    children.begin(), children.end(),
                    [&](UUID leftIdentifier, UUID rightIdentifier)
                    {
                        Entity leftEntity = GetEntityByUUID(leftIdentifier);
                        Entity rightEntity = GetEntityByUUID(rightIdentifier);
                        const uint32_t leftSiblingIndex =
                                leftEntity && leftEntity.HasComponent<RelationshipComponent>()
                                        ? leftEntity.GetComponent<RelationshipComponent>().SiblingIndex
                                        : 0;
                        const uint32_t rightSiblingIndex =
                                rightEntity && rightEntity.HasComponent<RelationshipComponent>()
                                        ? rightEntity.GetComponent<RelationshipComponent>().SiblingIndex
                                        : 0;
                        if (leftSiblingIndex != rightSiblingIndex)
                            return leftSiblingIndex < rightSiblingIndex;
                        return static_cast<uint64_t>(leftIdentifier)
                               < static_cast<uint64_t>(rightIdentifier);
                    });
        }
    }

    Entity Scene::GetParentEntity(Entity entity) const
    {
        if (!entity || !entity.HasComponent<RelationshipComponent>())
            return {};

        const UUID parentIdentifier = entity.GetComponent<RelationshipComponent>().Parent;
        if (parentIdentifier == 0)
            return {};

        return const_cast<Scene*>(this)->GetEntityByUUID(parentIdentifier);
    }

    const std::vector<UUID>& Scene::GetEntityChildren(Entity entity) const
    {
        if (!entity)
            return s_EmptyChildrenList;

        const auto iterator = m_ChildrenCache.find(entity.GetUUID());
        if (iterator == m_ChildrenCache.end())
            return s_EmptyChildrenList;

        return iterator->second;
    }

    std::vector<Entity> Scene::GetRootEntities(bool userInterfaceEntities) const
    {
        std::vector<Entity> rootEntities;
        auto tagView = m_Registry.view<TagComponent>();
        for (auto entityHandle : tagView)
        {
            Entity entity{entityHandle, const_cast<Scene*>(this)};
            const bool isUserInterfaceEntity = entity.HasComponent<RectTransformComponent>();
            if (isUserInterfaceEntity != userInterfaceEntities)
                continue;

            if (entity.HasComponent<RelationshipComponent>()
                && entity.GetComponent<RelationshipComponent>().Parent != 0)
                continue;

            rootEntities.push_back(entity);
        }

        return rootEntities;
    }

    bool Scene::IsEntityDescendantOf(Entity potentialDescendant, Entity potentialAncestor) const
    {
        if (!potentialDescendant || !potentialAncestor)
            return false;

        Entity currentEntity = GetParentEntity(potentialDescendant);
        while (currentEntity)
        {
            if (currentEntity == potentialAncestor)
                return true;
            currentEntity = GetParentEntity(currentEntity);
        }

        return false;
    }

    glm::mat4 Scene::GetEntityWorldTransformMatrix(Entity entity) const
    {
        if (!entity)
            return glm::mat4(1.0f);

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            if (transform.WorldTransformDirty)
            {
                Entity parentEntity = GetParentEntity(entity);
                if (parentEntity)
                {
                    transform.CachedWorldTransform =
                            GetEntityWorldTransformMatrix(parentEntity) * transform.GetLocalTransform();
                }
                else
                {
                    transform.CachedWorldTransform = transform.GetLocalTransform();
                }
                transform.WorldTransformDirty = false;
            }

            return transform.CachedWorldTransform;
        }

        if (entity.HasComponent<RectTransformComponent>())
        {
            float targetWidth = static_cast<float>(m_ViewportWidth);
            float targetHeight = static_cast<float>(m_ViewportHeight);
            Entity canvasEntity = FindCanvasEntity();
            if ((targetWidth <= 0.0f || targetHeight <= 0.0f)
                && canvasEntity && canvasEntity.HasComponent<CanvasComponent>())
            {
                const auto& canvas = canvasEntity.GetComponent<CanvasComponent>();
                targetWidth = canvas.ReferenceResolution.x;
                targetHeight = canvas.ReferenceResolution.y;
            }
            return ResolveRectTransform(entity, targetWidth, targetHeight).WorldTransform;
        }

        return glm::mat4(1.0f);
    }

    glm::vec3 Scene::GetEntityWorldTranslation(Entity entity) const
    {
        glm::vec3 translation{};
        glm::vec3 rotation{};
        glm::vec3 scale{};
        Math::DecomposeTransform(GetEntityWorldTransformMatrix(entity), translation, rotation, scale);
        return translation;
    }

    glm::vec3 Scene::GetEntityWorldRotation(Entity entity) const
    {
        glm::vec3 translation{};
        glm::vec3 rotation{};
        glm::vec3 scale{};
        Math::DecomposeTransform(GetEntityWorldTransformMatrix(entity), translation, rotation, scale);
        return rotation;
    }

    glm::vec3 Scene::GetEntityWorldScale(Entity entity) const
    {
        glm::vec3 translation{};
        glm::vec3 rotation{};
        glm::vec3 scale{};
        Math::DecomposeTransform(GetEntityWorldTransformMatrix(entity), translation, rotation, scale);
        return scale;
    }

    void Scene::ApplyMatrixAsLocalTransform(Entity entity, const glm::mat4& localMatrix)
    {
        glm::vec3 translation{};
        glm::vec3 rotation{};
        glm::vec3 scale{};
        Math::DecomposeTransform(localMatrix, translation, rotation, scale);

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.Position = translation;
            transform.Rotation = rotation;
            transform.Scale = scale;
        }
        else if (entity.HasComponent<RectTransformComponent>())
        {
            if (entity.HasComponent<CanvasComponent>())
            {
                // Canvas 根 Position/Rotation 锁定，忽略外部写回。
                SyncCanvasReferenceResolutionToTransform(entity);
                return;
            }

            auto& userInterfaceTransform = entity.GetComponent<RectTransformComponent>();
            userInterfaceTransform.AnchoredPosition = glm::vec2(translation);
            userInterfaceTransform.RotationRadians = rotation.z;
            // Size 不参与父子矩阵，分解出的 scale 不写回 Size（由编辑器 Gizmo 单独处理）。
        }
    }

    void Scene::ApplyWorldMatrixAsLocalTransform(Entity entity, const glm::mat4& worldMatrix)
    {
        Entity parentEntity = GetParentEntity(entity);
        const glm::mat4 parentWorldMatrix =
                parentEntity ? GetEntityWorldTransformMatrix(parentEntity) : glm::mat4(1.0f);
        ApplyMatrixAsLocalTransform(entity, glm::inverse(parentWorldMatrix) * worldMatrix);
    }

    void Scene::MarkEntityTransformDirty(Entity entity)
    {
        if (!entity)
            return;

        if (entity.HasComponent<TransformComponent>())
            entity.GetComponent<TransformComponent>().WorldTransformDirty = true;
        if (entity.HasComponent<RectTransformComponent>())
            entity.GetComponent<RectTransformComponent>().WorldTransformDirty = true;

        for (UUID childIdentifier : GetEntityChildren(entity))
        {
            Entity childEntity = GetEntityByUUID(childIdentifier);
            MarkEntityTransformDirty(childEntity);
        }
    }

    void Scene::NotifyEntityLocalTransformChanged(Entity entity)
    {
        MarkEntityTransformDirty(entity);
        SyncEntityTransformSubtreeToPhysics(entity);
    }

    void Scene::SyncEntityTransformSubtreeToPhysics(Entity entity)
    {
        if (!entity)
            return;

        if (entity.HasComponent<Rigidbody2DComponent>())
            SyncEntityTransformToPhysics(entity);

        for (UUID childIdentifier : GetEntityChildren(entity))
        {
            Entity childEntity = GetEntityByUUID(childIdentifier);
            SyncEntityTransformSubtreeToPhysics(childEntity);
        }
    }

    void Scene::ApplyPhysicsWorldTransform(Entity entity, const glm::vec2& worldPosition,
                                           float worldRotationZ)
    {
        if (!entity || !entity.HasComponent<TransformComponent>())
            return;

        Entity parentEntity = GetParentEntity(entity);
        if (!parentEntity)
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.Position.x = worldPosition.x;
            transform.Position.y = worldPosition.y;
            transform.Rotation.z = worldRotationZ;
        }
        else
        {
            glm::mat4 worldMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(worldPosition.x, worldPosition.y, 0.0f))
                                      * glm::rotate(glm::mat4(1.0f), worldRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));
            ApplyWorldMatrixAsLocalTransform(entity, worldMatrix);
        }

        MarkEntityTransformDirty(entity);
    }

    bool Scene::SetEntityParent(Entity child, Entity parent, bool keepWorldPosition)
    {
        if (!child)
            return false;
        if (child == parent)
            return false;
        if (parent && IsEntityDescendantOf(parent, child))
            return false;
        if (parent && !EntitiesShareTransformDomain(child, parent))
            return false;

        glm::mat4 worldMatrixBefore = glm::mat4(1.0f);
        if (keepWorldPosition)
            worldMatrixBefore = GetEntityWorldTransformMatrix(child);

        if (!parent)
        {
            if (child.HasComponent<RelationshipComponent>())
                child.RemoveComponent<RelationshipComponent>();
        }
        else
        {
            if (!child.HasComponent<RelationshipComponent>())
                child.AddComponent<RelationshipComponent>();
            auto& relationship = child.GetComponent<RelationshipComponent>();
            relationship.Parent = parent.GetUUID();
            relationship.SiblingIndex =
                    static_cast<uint32_t>(GetEntityChildren(parent).size());
        }

        RebuildHierarchyCache();
        MarkEntityTransformDirty(child);

        if (keepWorldPosition)
        {
            const glm::mat4 parentWorldMatrix =
                    parent ? GetEntityWorldTransformMatrix(parent) : glm::mat4(1.0f);
            ApplyMatrixAsLocalTransform(child, glm::inverse(parentWorldMatrix) * worldMatrixBefore);
        }

        MarkEntityTransformDirty(child);
        SyncEntityTransformSubtreeToPhysics(child);
        return true;
    }

    void Scene::UnparentEntity(Entity child, bool keepWorldPosition)
    {
        SetEntityParent(child, {}, keepWorldPosition);
    }

    //

}
