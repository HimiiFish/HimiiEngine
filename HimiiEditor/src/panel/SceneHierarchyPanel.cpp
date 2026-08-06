#include "SceneHierarchyPanel.h"
#include "commands/EditorCommandHistory.h"
#include "commands/EditorCommands.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include "World/Scene/PrefabSerializer.h"
#include "EngineCore/Utils/PlatformUtils.h"
#include "Resource/AssetManager.h"
#include "Module/Render/Renderer/Font.h"
#include "EngineCore/Core/Log.h"

#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstdio>

namespace Himii
{
    namespace
    {
        constexpr const char* kEntityDragDropPayloadType = "HIMII_ENTITY_UUID";

        bool ContainsCaseInsensitive(const std::string &text, const std::string &searchFilter)
        {
            if (searchFilter.empty())
                return true;

            auto toLowercaseCharacter = [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            };

            std::string lowercaseText(text.size(), '\0');
            std::transform(text.begin(), text.end(), lowercaseText.begin(), toLowercaseCharacter);

            std::string lowercaseFilter(searchFilter.size(), '\0');
            std::transform(searchFilter.begin(), searchFilter.end(), lowercaseFilter.begin(),
                           toLowercaseCharacter);
            return lowercaseText.find(lowercaseFilter) != std::string::npos;
        }
    }

    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        m_ComponentIcons["Transform"] = Texture2D::Create("resources/icons/Component_Transform.png");
        m_ComponentIcons["Camera"] = Texture2D::Create("resources/icons/Component_Camera.png");
        m_ComponentIcons["Light"] = Texture2D::Create("resources/icons/Component_Light.png");
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
            DrawCreateEntityMenu();
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
                DrawAddComponentMenu(true);
                ImGui::EndPopup();
            }
        }

        EndInspectorPropertiesStyle();
        ImGui::End();
    }

    std::string SceneHierarchyPanel::BuildUniqueSiblingName(const std::string &baseName, Entity parentEntity,
                                                            bool userInterfaceEntity) const
    {
        if (!m_Context)
            return baseName;

        auto siblingNameExists = [&](const std::string &candidateName)
        {
            if (parentEntity)
            {
                for (UUID childIdentifier : m_Context->GetEntityChildren(parentEntity))
                {
                    Entity childEntity = m_Context->GetEntityByUUID(childIdentifier);
                    if (childEntity && childEntity.GetComponent<TagComponent>().Tag == candidateName)
                        return true;
                }
                return false;
            }

            for (Entity rootEntity : m_Context->GetRootEntities(userInterfaceEntity))
            {
                if (rootEntity.GetComponent<TagComponent>().Tag == candidateName)
                    return true;
            }
            return false;
        };

        if (!siblingNameExists(baseName))
            return baseName;

        for (uint32_t nameIndex = 1; ; ++nameIndex)
        {
            const std::string candidateName =
                    baseName + " (" + std::to_string(nameIndex) + ")";
            if (!siblingNameExists(candidateName))
                return candidateName;
        }
    }

    void SceneHierarchyPanel::CreateEntityFromMenu(
            const std::string &baseName,
            const std::function<Entity(const Ref<Scene>&, const std::string&)> &createEntityFunction,
            Entity parentEntity)
    {
        if (!m_Context)
            return;

        const bool userInterfaceEntity = parentEntity
                && parentEntity.HasComponent<RectTransformComponent>();
        Entity siblingParent = parentEntity;
        if (!siblingParent && userInterfaceEntity)
            siblingParent = m_Context->FindCanvasEntity();

        const std::string uniqueName =
                BuildUniqueSiblingName(baseName, siblingParent, userInterfaceEntity);
        const UUID parentIdentifier = parentEntity ? parentEntity.GetUUID() : UUID(0);

        auto createAndSelectEntity =
                [this, createEntityFunction, uniqueName, parentIdentifier](const Ref<Scene> &scene)
                {
                    Entity createdEntity = createEntityFunction(scene, uniqueName);
                    if (!createdEntity)
                        return Entity{};

                    Entity resolvedParent;
                    if (static_cast<uint64_t>(parentIdentifier) != 0)
                    {
                        resolvedParent = scene->GetEntityByUUID(parentIdentifier);
                    }
                    else if (createdEntity.HasComponent<RectTransformComponent>()
                             && !createdEntity.HasComponent<CanvasComponent>())
                    {
                        resolvedParent = scene->FindCanvasEntity();
                        if (!resolvedParent)
                            resolvedParent = scene->CreateCanvasEntity("Canvas");
                    }

                    if (resolvedParent)
                        scene->SetEntityParent(createdEntity, resolvedParent, false);

                    m_SelectionContext = createdEntity;
                    return createdEntity;
                };

        if (m_CommandHistory)
        {
            m_CommandHistory->Execute(
                    CreateScope<CreateEntityCommand>(m_Context, createAndSelectEntity));
        }
        else
        {
            createAndSelectEntity(m_Context);
        }
    }

    void SceneHierarchyPanel::DrawCreateEntityMenu(Entity parentEntity)
    {
        if (!m_Context)
            return;

        const bool hasParent = static_cast<bool>(parentEntity);
        const bool parentIsUserInterface =
                hasParent && parentEntity.HasComponent<RectTransformComponent>();
        const bool showWorldEntities = !hasParent || !parentIsUserInterface;
        const bool showUserInterfaceEntities = !hasParent || parentIsUserInterface;
        const Entity userInterfaceParent = parentIsUserInterface
                ? parentEntity
                : (!hasParent ? m_Context->FindCanvasEntity() : Entity{});

        if (ImGui::MenuItem("Create Empty"))
        {
            if (parentIsUserInterface)
            {
                CreateEntityFromMenu(
                        "Empty UI Entity",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            return scene->CreateUIEntity(name);
                        },
                        userInterfaceParent);
            }
            else
            {
                CreateEntityFromMenu(
                        "Empty Entity",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            return scene->CreateEntity(name);
                        },
                        parentEntity);
            }
        }

        if (showWorldEntities && ImGui::BeginMenu("2D Object"))
        {
            if (ImGui::MenuItem("Sprite"))
            {
                CreateEntityFromMenu(
                        "Sprite",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.AddComponent<SpriteRendererComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            if (ImGui::MenuItem("Circle"))
            {
                CreateEntityFromMenu(
                        "Circle",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.AddComponent<CircleRendererComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            if (ImGui::MenuItem("Tilemap"))
            {
                CreateEntityFromMenu(
                        "Tilemap",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.AddComponent<TilemapComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            ImGui::EndMenu();
        }

        if (showWorldEntities && ImGui::BeginMenu("3D Object"))
        {
            const auto drawMeshCreationEntry =
                    [this, parentEntity](const char *label, MeshComponent::MeshType meshType)
                    {
                        if (!ImGui::MenuItem(label))
                            return;
                        CreateEntityFromMenu(
                                label,
                                [meshType](const Ref<Scene> &scene, const std::string &name)
                                {
                                    Entity entity = scene->CreateEntity(name);
                                    entity.AddComponent<MeshComponent>().Type = meshType;
                                    return entity;
                                },
                                parentEntity);
                    };

            drawMeshCreationEntry("Cube", MeshComponent::MeshType::Cube);
            drawMeshCreationEntry("Plane", MeshComponent::MeshType::Plane);
            drawMeshCreationEntry("Sphere", MeshComponent::MeshType::Sphere);
            drawMeshCreationEntry("Capsule", MeshComponent::MeshType::Capsule);
            ImGui::EndMenu();
        }

        if (showWorldEntities && ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                CreateEntityFromMenu(
                        "Directional Light",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.GetComponent<TransformComponent>().Rotation =
                                    glm::radians(glm::vec3(50.0f, -30.0f, 0.0f));
                            entity.AddComponent<LightComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                CreateEntityFromMenu(
                        "Point Light",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            auto &lightComponent = entity.AddComponent<LightComponent>();
                            lightComponent.Type = LightType::Point;
                            lightComponent.Range = 10.0f;
                            lightComponent.CastShadows = false;
                            return entity;
                        },
                        parentEntity);
            }
            if (ImGui::MenuItem("Environment"))
            {
                CreateEntityFromMenu(
                        "Environment",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.AddComponent<EnvironmentComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            ImGui::EndMenu();
        }

        if (showWorldEntities && ImGui::BeginMenu("Visual Effects"))
        {
            if (ImGui::MenuItem("Particle Emitter"))
            {
                CreateEntityFromMenu(
                        "Particle Emitter",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateEntity(name);
                            entity.AddComponent<ParticleEmitterComponent>();
                            return entity;
                        },
                        parentEntity);
            }
            ImGui::EndMenu();
        }

        if (showWorldEntities && ImGui::MenuItem("Camera"))
        {
            CreateEntityFromMenu(
                    "Camera",
                    [](const Ref<Scene> &scene, const std::string &name)
                    {
                        Entity entity = scene->CreateEntity(name);
                        entity.AddComponent<CameraComponent>();
                        return entity;
                    },
                    parentEntity);
        }

        if (showUserInterfaceEntities && ImGui::BeginMenu("UI"))
        {
            const bool sceneHasCanvas = static_cast<bool>(m_Context->FindCanvasEntity());
            if (!parentIsUserInterface)
            {
                ImGui::BeginDisabled(sceneHasCanvas);
                if (ImGui::MenuItem("Canvas"))
                {
                    CreateEntityFromMenu(
                            "Canvas",
                            [](const Ref<Scene> &scene, const std::string &name)
                            {
                                return scene->CreateCanvasEntity(name);
                            });
                }
                ImGui::EndDisabled();
            }

            if (ImGui::MenuItem("Text"))
            {
                CreateEntityFromMenu(
                        "Text",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            Entity entity = scene->CreateUIEntity(name);
                            auto &text = entity.AddComponent<UITextComponent>();
                            text.FontAsset = Font::GetDefault();
                            text.FontSize = 48.0f;
                            auto &rectTransform = entity.GetComponent<RectTransformComponent>();
                            rectTransform.SizeDelta = glm::vec2(300.0f, 100.0f);
                            rectTransform.ResolvedSize = rectTransform.SizeDelta;
                            return entity;
                        },
                        userInterfaceParent);
            }
            if (ImGui::MenuItem("Button"))
            {
                CreateEntityFromMenu(
                        "Button",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            return scene->CreateUIButtonEntity(name);
                        },
                        userInterfaceParent);
            }
            if (ImGui::MenuItem("Empty UI"))
            {
                CreateEntityFromMenu(
                        "Empty UI Entity",
                        [](const Ref<Scene> &scene, const std::string &name)
                        {
                            return scene->CreateUIEntity(name);
                        },
                        userInterfaceParent);
            }
            ImGui::EndMenu();
        }
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
            if (ImGui::BeginMenu("Create"))
            {
                DrawCreateEntityMenu(entity);
                ImGui::EndMenu();
            }

            ImGui::Separator();
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
        {
            m_AddComponentSearchBuffer.fill('\0');
            ImGui::OpenPopup("AddComponent");
        }

        if (ImGui::BeginPopup("AddComponent"))
        {
            ImGui::SetNextItemWidth(280.0f);
            ImGui::InputTextWithHint("##AddComponentSearch", "Search components...",
                                     m_AddComponentSearchBuffer.data(),
                                     m_AddComponentSearchBuffer.size());
            ImGui::Separator();

            const std::string searchFilter = m_AddComponentSearchBuffer.data();
            DrawAddComponentMenu(searchFilter.empty(), searchFilter);

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

        componentInspectorDrawContext.requestMaterialEditor = [this](AssetHandle handle)
        {
            m_MaterialEditorRequest = handle;
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

    void SceneHierarchyPanel::DrawAddComponentMenu(bool useGroupedMenus,
                                                   const std::string &searchFilter)
    {
        if (!m_SelectionContext)
            return;

        const bool userInterfaceEntity =
                m_SelectionContext.HasComponent<RectTransformComponent>();

        const auto drawRenderingEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<CameraComponent>("Camera", searchFilter);
            DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer", searchFilter);
            DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer", searchFilter);
            DisplayAddComponentEntry<MeshComponent>("Mesh Renderer", searchFilter);
            DisplayAddComponentEntry<LightComponent>("Light", searchFilter);
            DisplayAddComponentEntry<EnvironmentComponent>("Environment", searchFilter);
            DisplayAddComponentEntry<ParticleEmitterComponent>("Particle Emitter", searchFilter);
        };
        const auto drawPhysicsEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D", searchFilter);
            DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D", searchFilter);
            DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D", searchFilter);
        };
        const auto drawAnimationEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<SpriteAnimationComponent>("Sprite Animation", searchFilter);
        };
        const auto drawTilemapEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<TilemapComponent>("Tilemap", searchFilter);
            DisplayAddComponentEntry<TilemapCollider2DComponent>("Tilemap Collider 2D", searchFilter);
        };
        const auto drawScriptingEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<ScriptComponent>("Script", searchFilter);
        };
        const auto drawAudioEntries = [this, &searchFilter]()
        {
            DisplayAddComponentEntry<SoundPlayerComponent>("Sound Player", searchFilter);
        };
        const auto drawUserInterfaceEntries = [this, &searchFilter]()
        {
            if (!m_Context->FindCanvasEntity())
                DisplayAddComponentEntry<CanvasComponent>("Canvas", searchFilter);
            DisplayAddComponentEntry<UIImageComponent>("Image", searchFilter);
            DisplayAddComponentEntry<UITextComponent>("Text", searchFilter);
            DisplayAddComponentEntry<UIButtonComponent>("Button", searchFilter);
        };

        if (!useGroupedMenus)
        {
            if (userInterfaceEntity)
            {
                drawUserInterfaceEntries();
                drawScriptingEntries();
                drawAudioEntries();
            }
            else
            {
                drawRenderingEntries();
                drawPhysicsEntries();
                drawAnimationEntries();
                drawTilemapEntries();
                drawScriptingEntries();
                drawAudioEntries();
            }
            return;
        }

        if (userInterfaceEntity)
        {
            if (ImGui::BeginMenu("User Interface"))
            {
                drawUserInterfaceEntries();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Scripting"))
            {
                drawScriptingEntries();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Audio"))
            {
                drawAudioEntries();
                ImGui::EndMenu();
            }
            return;
        }

        if (ImGui::BeginMenu("Rendering"))
        {
            drawRenderingEntries();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Physics 2D"))
        {
            drawPhysicsEntries();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Animation"))
        {
            drawAnimationEntries();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tilemap"))
        {
            drawTilemapEntries();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Scripting"))
        {
            drawScriptingEntries();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Audio"))
        {
            drawAudioEntries();
            ImGui::EndMenu();
        }
    }

    template<typename T>
    void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string &entryName,
                                                       const std::string &searchFilter)
    {
        if (!m_SelectionContext.HasComponent<T>()
            && ContainsCaseInsensitive(entryName, searchFilter))
        {
            if (ImGui::MenuItem(entryName.c_str()))
            {
                m_SelectionContext.AddComponent<T>();
                ImGui::CloseCurrentPopup();
            }
        }
    }

}
