#include "SceneHierarchyPanel.h"
#include "commands/EditorCommandHistory.h"
#include "commands/EditorCommands.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include "World/Scene/PrefabSerializer.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "Resource/AssetManager.h"
#include "Module/Render/Font.h"
#include "EngineCore/Core/Log.h"

#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include <imgui.h>
#include <filesystem>
#include <cstdio>

namespace Himii
{
    namespace
    {
        constexpr const char* kEntityDragDropPayloadType = "HIMII_ENTITY_UUID";
    }

    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        m_ComponentIcons["Transform"] = Texture2D::Create("resources/icons/Component_Transform.png");
        m_ComponentIcons["Camera"] = Texture2D::Create("resources/icons/Component_Camera.png");
        m_ComponentIcons["Script"] = Texture2D::Create("resources/icons/Component_Script.png");
        m_ComponentIcons["Sprite Renderer"] = Texture2D::Create("resources/icons/Component_SpriteRenderer.png");
        m_ComponentIcons["Circle Renderer"] = Texture2D::Create("resources/icons/Component_CircleRenderer.png");
        m_ComponentIcons["Rigidbody2D"] = Texture2D::Create("resources/icons/Component_Rigidbody.png");
        m_ComponentIcons["Box Collider2D"] = Texture2D::Create("resources/icons/Component_BoxCollider.png");
        m_ComponentIcons["Circle Collider2D"] = Texture2D::Create("resources/icons/Component_CircleCollider.png");
        m_ComponentIcons["Sprite Animation"] = Texture2D::Create("resources/icons/Component_Animator.png");
        m_ComponentIcons["Mesh Renderer"] = Texture2D::Create("resources/icons/Component_SpriteRenderer.png");
    }

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
        : SceneHierarchyPanel()
    {
        SetContext(context);
    }

    void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
    {
        m_Context = context;
        if (m_Context)
            m_Context->RebuildHierarchyCache();
    }

    void SceneHierarchyPanel::SetCommandHistory(EditorCommandHistory* commandHistory)
    {
        m_CommandHistory = commandHistory;
    }

    void SceneHierarchyPanel::DrawHierarchyRoots(bool userInterfaceEntities)
    {
        if (!m_Context)
            return;

        for (Entity rootEntity : m_Context->GetRootEntities(userInterfaceEntities))
            DrawEntityNode(rootEntity);
    }

    void SceneHierarchyPanel::HandleEntityReparent(Entity draggedEntity, Entity newParentEntity)
    {
        if (!m_Context || !draggedEntity)
            return;
        if (draggedEntity == newParentEntity)
            return;

        if (newParentEntity && !m_Context->EntitiesShareTransformDomain(draggedEntity, newParentEntity))
            return;

        if (m_CommandHistory)
        {
            m_CommandHistory->Execute(
                    CreateScope<ReparentEntityCommand>(m_Context, draggedEntity, newParentEntity, true));
        }
        else
        {
            m_Context->SetEntityParent(draggedEntity, newParentEntity, true);
        }
    }

    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);

        if (m_Context)
        {
            DrawHierarchyRoots(false);
            DrawHierarchyRoots(true);
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kEntityDragDropPayloadType))
            {
                UUID draggedEntityIdentifier = *static_cast<const UUID*>(payload->Data);
                Entity draggedEntity = m_Context->GetEntityByUUID(draggedEntityIdentifier);
                HandleEntityReparent(draggedEntity, {});
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginPopupContextWindow(0, 1))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene) { return scene->CreateEntity("Empty Entity"); }));
                }
                else
                    m_Context->CreateEntity("Empty Entity");
            }
            if (ImGui::MenuItem("Create UI Canvas"))
            {
                if (m_Context->FindCanvasEntity())
                {
                    HIMII_CORE_WARNING("Scene already has a Canvas.");
                }
                else if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene) { return scene->CreateCanvasEntity("Canvas"); }));
                }
                else
                    m_Context->CreateCanvasEntity("Canvas");
            }
            if (ImGui::MenuItem("Create UI Text"))
            {
                auto createTextEntity = [](const Ref<Scene>& scene) -> Entity
                {
                    Entity canvasEntity = scene->FindCanvasEntity();
                    if (!canvasEntity)
                        canvasEntity = scene->CreateCanvasEntity("Canvas");
                    if (!canvasEntity)
                        return {};

                    Entity textEntity = scene->CreateUIEntity("Text");
                    auto& text = textEntity.AddComponent<UITextComponent>();
                    text.FontAsset = Font::GetDefault();
                    text.FontSize = 48.0f;

                    auto& userInterfaceTransform = textEntity.GetComponent<RectTransformComponent>();
                    userInterfaceTransform.AnchoredPosition = glm::vec2(0.0f);
                    userInterfaceTransform.SizeDelta = glm::vec2(300.0f, 100.0f);
                    userInterfaceTransform.ResolvedSize = userInterfaceTransform.SizeDelta;

                    scene->SetEntityParent(textEntity, canvasEntity, false);
                    return textEntity;
                };

                if (m_CommandHistory)
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(m_Context, createTextEntity));
                else
                    createTextEntity(m_Context);
            }
            if (ImGui::MenuItem("Create UI Button"))
            {
                auto createButtonEntity = [](const Ref<Scene>& scene) -> Entity
                {
                    Entity canvasEntity = scene->FindCanvasEntity();
                    if (!canvasEntity)
                        canvasEntity = scene->CreateCanvasEntity("Canvas");

                    Entity buttonEntity = scene->CreateUIButtonEntity("Button");
                    scene->SetEntityParent(buttonEntity, canvasEntity, false);
                    return buttonEntity;
                };

                if (m_CommandHistory)
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(m_Context, createButtonEntity));
                else
                    createButtonEntity(m_Context);
            }
            if (ImGui::MenuItem("Create UI Entity"))
            {
                if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene) { return scene->CreateUIEntity("Empty UI Entity"); }));
                }
                else
                    m_Context->CreateUIEntity("Empty UI Entity");
            }
            if (ImGui::MenuItem("Create Cube"))
            {
                if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene)
                        {
                            Entity entity = scene->CreateEntity("Cube");
                            entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Cube;
                            return entity;
                        }));
                }
                else
                {
                    auto entity = m_Context->CreateEntity("Cube");
                    entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Cube;
                }
            }
            if (ImGui::MenuItem("Create Sphere"))
            {
                if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene)
                        {
                            Entity entity = scene->CreateEntity("Sphere");
                            entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Sphere;
                            return entity;
                        }));
                }
                else
                {
                    auto entity = m_Context->CreateEntity("Sphere");
                    entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Sphere;
                }
            }
            if (ImGui::MenuItem("Create Capsule"))
            {
                if (m_CommandHistory)
                {
                    m_CommandHistory->Execute(CreateScope<CreateEntityCommand>(
                        m_Context,
                        [](const Ref<Scene>& scene)
                        {
                            Entity entity = scene->CreateEntity("Capsule");
                            entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Capsule;
                            return entity;
                        }));
                }
                else
                {
                    auto entity = m_Context->CreateEntity("Capsule");
                    entity.AddComponent<MeshComponent>().Type = MeshComponent::MeshType::Capsule;
                }
            }
            ImGui::EndPopup();
        }
        ImGui::End();

        ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse);
        BeginInspectorPropertiesStyle();

        if (m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);

            if (ImGui::BeginPopupContextWindow(0, 1))
            {
                DisplayAddComponentEntry<CameraComponent>("Camera");
                DisplayAddComponentEntry<ScriptComponent>("Script Component");
                DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
                DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
                DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody2D");
                DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider2D");
                DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
                DisplayAddComponentEntry<MeshComponent>("Mesh Renderer");
                DisplayAddComponentEntry<TilemapComponent>("Tilemap");
                DisplayAddComponentEntry<TilemapCollider2DComponent>("Tilemap Collider 2D");
                DisplayAddComponentEntry<ParticleEmitterComponent>("Particle Emitter");
                DisplayAddComponentEntry<RectTransformComponent>("Rect Transform");
                DisplayAddComponentEntry<UIImageComponent>("Image");
                DisplayAddComponentEntry<UIButtonComponent>("Button");
                DisplayAddComponentEntry<SoundPlayerComponent>("Sound Player");

                ImGui::EndPopup();
            }
        }

        EndInspectorPropertiesStyle();
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        if (!entity)
            return;

        auto &tag = entity.GetComponent<TagComponent>().Tag;
        const std::vector<UUID>& children = m_Context->GetEntityChildren(entity);
        const bool hasChildren = !children.empty();
        const bool isUserInterfaceEntity = entity.HasComponent<RectTransformComponent>();
        const bool isOrphanUserInterface =
                isUserInterfaceEntity && !m_Context->IsEntityUnderCanvas(entity);

        ImGuiTreeNodeFlags flags =
                ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!hasChildren)
            flags |= ImGuiTreeNodeFlags_Leaf;

        if (isOrphanUserInterface)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.2f, 1.0f));

        const UUID entityIdentifier = entity.GetUUID();
        bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(entityIdentifier)),
                                        flags, "%s%s", tag.c_str(),
                                        isOrphanUserInterface ? " (needs Canvas)" : "");
        if (isOrphanUserInterface)
            ImGui::PopStyleColor();

        if (ImGui::IsItemHovered() && isOrphanUserInterface)
            ImGui::SetTooltip("UI entities only render under a Canvas. Drag onto Canvas.");

        if (ImGui::IsItemClicked())
            m_SelectionContext = entity;

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload(kEntityDragDropPayloadType, &entityIdentifier, sizeof(UUID));
            ImGui::TextUnformatted(tag.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kEntityDragDropPayloadType))
            {
                UUID draggedEntityIdentifier = *static_cast<const UUID*>(payload->Data);
                Entity draggedEntity = m_Context->GetEntityByUUID(draggedEntityIdentifier);
                HandleEntityReparent(draggedEntity, entity);
            }
            ImGui::EndDragDropTarget();
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Save as Prefab..."))
            {
                std::string filePath = FileDialog::SaveFile("Himii Prefab (*.hprefab)\0*.hprefab\0");
                if (!filePath.empty())
                {
                    if (PrefabSerializer::Save(entity, filePath))
                    {
                        if (Project::GetActive())
                        {
                            std::filesystem::path relativePath =
                                    std::filesystem::relative(filePath, Project::GetAssetDirectory());
                            if (auto assetManager = ResourceSystem::GetAssetManager())
                                assetManager->ImportAsset(relativePath);
                        }
                    }
                }
            }

            if (ImGui::MenuItem("Unparent"))
            {
                HandleEntityReparent(entity, {});
            }

            if (ImGui::MenuItem("Delete Entity"))
                entityDeleted = true;

            ImGui::EndPopup();
        }

        if (opened)
        {
            for (UUID childIdentifier : children)
                DrawEntityNode(m_Context->GetEntityByUUID(childIdentifier));
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            if (m_CommandHistory)
            {
                auto onEntityRestored = [this](Entity restoredEntity)
                {
                    m_SelectionContext = restoredEntity;
                };
                m_CommandHistory->Execute(
                    CreateScope<DeleteEntityCommand>(m_Context, entity, onEntityRestored));
            }
            else
                m_Context->DestroyEntity(entity);

            if (m_SelectionContext == entity)
                m_SelectionContext = {};
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto &tag = entity.GetComponent<TagComponent>().Tag;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));

            strcat(buffer, tag.c_str());
            std::string tagBeforeInput = tag;
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
                tag = std::string(buffer);

            if (ImGui::IsItemActivated())
                m_TagEditStartValue = tagBeforeInput;

            if (ImGui::IsItemDeactivatedAfterEdit() && m_CommandHistory)
            {
                if (tag != m_TagEditStartValue)
                {
                    m_CommandHistory->Execute(CreateScope<ModifyEntityTagCommand>(
                        m_Context, entity.GetUUID(), m_TagEditStartValue, tag));
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<CameraComponent>("Camera");
            DisplayAddComponentEntry<ScriptComponent>("Script Component");
            DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
            DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
            DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody2D");
            DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
            DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");
            DisplayAddComponentEntry<SpriteAnimationComponent>("Sprite Animation");
            DisplayAddComponentEntry<MeshComponent>("Mesh Renderer");
            DisplayAddComponentEntry<TilemapComponent>("Tilemap");
            DisplayAddComponentEntry<TilemapCollider2DComponent>("Tilemap Collider 2D");
            DisplayAddComponentEntry<ParticleEmitterComponent>("Particle Emitter");

            DisplayAddComponentEntry<RectTransformComponent>("Rect Transform");
            DisplayAddComponentEntry<UIImageComponent>("Image");
            DisplayAddComponentEntry<UITextComponent>("Text");
            DisplayAddComponentEntry<UIButtonComponent>("Button");
            DisplayAddComponentEntry<SoundPlayerComponent>("Sound Player");
            if (!m_Context->FindCanvasEntity())
                DisplayAddComponentEntry<CanvasComponent>("Canvas");

            ImGui::EndPopup();
        }

        ComponentInspectorDrawContext componentInspectorDrawContext;
        componentInspectorDrawContext.scene = m_Context;
        componentInspectorDrawContext.entity = entity;
        componentInspectorDrawContext.commandHistory = m_CommandHistory;

        componentInspectorDrawContext.getComponentIcon = [this](const std::string& iconKey)
        {
            auto iterator = m_ComponentIcons.find(iconKey);
            if (iterator != m_ComponentIcons.end())
                return iterator->second;
            return Ref<Texture2D>{};
        };

        componentInspectorDrawContext.requestTextureInspector = [this](AssetHandle handle)
        {
            m_TextureInspectorRequest = handle;
        };

        componentInspectorDrawContext.requestParticleEmitterEditor = [this](AssetHandle handle)
        {
            m_ParticleEmitterEditorRequest = handle;
        };

        componentInspectorDrawContext.requestAnimationEditor =
            [this](const std::filesystem::path& animationEditorPath)
        {
            m_AnimationEditorRequest = animationEditorPath;
        };

        componentInspectorDrawContext.requestTileMapEditor = [this](AssetHandle handle)
        {
            m_TileMapEditorRequest = handle;
        };

        ComponentInspectorRegistry::Get().DrawAll(componentInspectorDrawContext);
    }

    template<typename T>
    void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string &entryName)
    {
        if (!m_SelectionContext.HasComponent<T>())
        {
            if (ImGui::MenuItem(entryName.c_str()))
            {
                m_SelectionContext.AddComponent<T>();
                ImGui::CloseCurrentPopup();
            }
        }
    }

}
