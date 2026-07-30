#include "EditorCommands.h"

#include <algorithm>

namespace Himii
{
    CreateEntityCommand::CreateEntityCommand(const Ref<Scene>& scene,
                                             CreateEntityFunction createEntityFunction)
        : m_Scene(scene)
        , m_CreateEntityFunction(std::move(createEntityFunction))
    {
    }

    void CreateEntityCommand::Execute()
    {
        if (!m_EntitySnapshotYaml.empty())
        {
            EntitySnapshot::Restore(m_Scene, m_EntitySnapshotYaml);
            return;
        }

        Entity createdEntity = m_CreateEntityFunction(m_Scene);
        if (createdEntity)
            m_EntityIdentifier = createdEntity.GetUUID();
    }

    void CreateEntityCommand::Undo()
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (!entity)
            return;

        m_EntitySnapshotYaml = EntitySnapshot::Serialize(entity);
        m_Scene->DestroyEntity(entity);
    }

    DeleteEntityCommand::DeleteEntityCommand(const Ref<Scene>& scene, Entity entity,
                                             std::function<void(Entity)> onEntityRestored)
        : m_Scene(scene)
        , m_EntityIdentifier(entity.GetUUID())
        , m_OnEntityRestored(std::move(onEntityRestored))
    {
        m_EntitySnapshotYaml = EntitySnapshot::SerializeSubtree(m_Scene, entity);
    }

    void DeleteEntityCommand::Execute()
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (entity)
            m_Scene->DestroyEntity(entity);
    }

    void DeleteEntityCommand::Undo()
    {
        EntitySnapshot::RestoreSubtree(m_Scene, m_EntitySnapshotYaml);
        Entity restoredEntity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (restoredEntity && m_OnEntityRestored)
            m_OnEntityRestored(restoredEntity);
    }

    ModifyTransformCommand::ModifyTransformCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                                                   const TransformComponent& beforeTransform,
                                                   const TransformComponent& afterTransform)
        : m_Scene(scene)
        , m_EntityIdentifier(entityIdentifier)
        , m_BeforeTransform(beforeTransform)
        , m_AfterTransform(afterTransform)
    {
    }

    void ModifyTransformCommand::ApplyTransform(const TransformComponent& transform)
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (entity && entity.HasComponent<TransformComponent>())
        {
            entity.GetComponent<TransformComponent>() = transform;
            entity.GetComponent<TransformComponent>().WorldTransformDirty = true;
            m_Scene->NotifyEntityLocalTransformChanged(entity);
        }
    }

    void ModifyTransformCommand::Execute()
    {
        ApplyTransform(m_AfterTransform);
    }

    void ModifyTransformCommand::Undo()
    {
        ApplyTransform(m_BeforeTransform);
    }

    bool ModifyTransformCommand::TryMerge(const IEditorCommand& other)
    {
        const auto* otherCommand = dynamic_cast<const ModifyTransformCommand*>(&other);
        if (!otherCommand || otherCommand->m_EntityIdentifier != m_EntityIdentifier)
            return false;

        m_AfterTransform = otherCommand->m_AfterTransform;
        return true;
    }

    ModifyRectTransformCommand::ModifyRectTransformCommand(
            const Ref<Scene>& scene, UUID entityIdentifier,
            const RectTransformComponent& beforeTransform,
            const RectTransformComponent& afterTransform)
        : m_Scene(scene)
        , m_EntityIdentifier(entityIdentifier)
        , m_BeforeTransform(beforeTransform)
        , m_AfterTransform(afterTransform)
    {
    }

    void ModifyRectTransformCommand::ApplyTransform(const RectTransformComponent& transform)
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (entity && entity.HasComponent<RectTransformComponent>())
        {
            entity.GetComponent<RectTransformComponent>() = transform;
            entity.GetComponent<RectTransformComponent>().WorldTransformDirty = true;
            m_Scene->NotifyEntityLocalTransformChanged(entity);
        }
    }

    void ModifyRectTransformCommand::Execute()
    {
        ApplyTransform(m_AfterTransform);
    }

    void ModifyRectTransformCommand::Undo()
    {
        ApplyTransform(m_BeforeTransform);
    }

    bool ModifyRectTransformCommand::TryMerge(const IEditorCommand& other)
    {
        const auto* otherCommand = dynamic_cast<const ModifyRectTransformCommand*>(&other);
        if (!otherCommand || otherCommand->m_EntityIdentifier != m_EntityIdentifier)
            return false;

        m_AfterTransform = otherCommand->m_AfterTransform;
        return true;
    }

    ModifyUITextFontSizeCommand::ModifyUITextFontSizeCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                                                             float beforeFontSize, float afterFontSize)
        : m_Scene(scene)
        , m_EntityIdentifier(entityIdentifier)
        , m_BeforeFontSize(beforeFontSize)
        , m_AfterFontSize(afterFontSize)
    {
    }

    void ModifyUITextFontSizeCommand::ApplyFontSize(float fontSize)
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (entity && entity.HasComponent<UITextComponent>())
            entity.GetComponent<UITextComponent>().FontSize = std::max(fontSize, 1.0f);
    }

    void ModifyUITextFontSizeCommand::Execute()
    {
        ApplyFontSize(m_AfterFontSize);
    }

    void ModifyUITextFontSizeCommand::Undo()
    {
        ApplyFontSize(m_BeforeFontSize);
    }

    bool ModifyUITextFontSizeCommand::TryMerge(const IEditorCommand& other)
    {
        const auto* otherCommand = dynamic_cast<const ModifyUITextFontSizeCommand*>(&other);
        if (!otherCommand || otherCommand->m_EntityIdentifier != m_EntityIdentifier)
            return false;

        m_AfterFontSize = otherCommand->m_AfterFontSize;
        return true;
    }

    ModifyEntityTagCommand::ModifyEntityTagCommand(const Ref<Scene>& scene, UUID entityIdentifier,
                                                   std::string beforeTag, std::string afterTag)
        : m_Scene(scene)
        , m_EntityIdentifier(entityIdentifier)
        , m_BeforeTag(std::move(beforeTag))
        , m_AfterTag(std::move(afterTag))
    {
    }

    void ModifyEntityTagCommand::ApplyTag(const std::string& tag)
    {
        Entity entity = m_Scene->GetEntityByUUID(m_EntityIdentifier);
        if (entity && entity.HasComponent<TagComponent>())
            entity.GetComponent<TagComponent>().Tag = tag;
    }

    void ModifyEntityTagCommand::Execute()
    {
        ApplyTag(m_AfterTag);
    }

    void ModifyEntityTagCommand::Undo()
    {
        ApplyTag(m_BeforeTag);
    }

    ReparentEntityCommand::ReparentEntityCommand(const Ref<Scene>& scene, Entity child, Entity newParent,
                                                 bool keepWorldPosition)
        : m_Scene(scene)
        , m_ChildIdentifier(child.GetUUID())
        , m_ParentAfterIdentifier(newParent ? newParent.GetUUID() : UUID(0))
        , m_KeepWorldPosition(keepWorldPosition)
    {
        m_IsUserInterface = child.HasComponent<RectTransformComponent>();
        Entity parentBefore = scene->GetParentEntity(child);
        m_ParentBeforeIdentifier = parentBefore ? parentBefore.GetUUID() : UUID(0);

        if (m_IsUserInterface)
            m_RectTransformBefore = child.GetComponent<RectTransformComponent>();
        else
            m_LocalTransformBefore = child.GetComponent<TransformComponent>();
    }

    void ReparentEntityCommand::ApplyParent(UUID parentIdentifier,
                                            const TransformComponent& localTransform,
                                            const RectTransformComponent& rectTransform,
                                            bool isUserInterface)
    {
        Entity childEntity = m_Scene->GetEntityByUUID(m_ChildIdentifier);
        if (!childEntity)
            return;

        Entity parentEntity = parentIdentifier != 0 ? m_Scene->GetEntityByUUID(parentIdentifier) : Entity{};
        if (isUserInterface)
            childEntity.GetComponent<RectTransformComponent>() = rectTransform;
        else
            childEntity.GetComponent<TransformComponent>() = localTransform;

        m_Scene->SetEntityParent(childEntity, parentEntity, false);
    }

    void ReparentEntityCommand::Execute()
    {
        Entity childEntity = m_Scene->GetEntityByUUID(m_ChildIdentifier);
        Entity parentEntity =
                m_ParentAfterIdentifier != 0 ? m_Scene->GetEntityByUUID(m_ParentAfterIdentifier) : Entity{};
        if (!childEntity)
            return;

        m_Scene->SetEntityParent(childEntity, parentEntity, m_KeepWorldPosition);

        if (m_IsUserInterface)
            m_RectTransformAfter = childEntity.GetComponent<RectTransformComponent>();
        else
            m_LocalTransformAfter = childEntity.GetComponent<TransformComponent>();
    }

    void ReparentEntityCommand::Undo()
    {
        if (m_IsUserInterface)
            ApplyParent(m_ParentBeforeIdentifier, {}, m_RectTransformBefore, true);
        else
            ApplyParent(m_ParentBeforeIdentifier, m_LocalTransformBefore, {}, false);
    }
}
