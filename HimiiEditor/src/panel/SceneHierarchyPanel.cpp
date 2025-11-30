#pragma once
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "filesystem"

#include <glm/gtc/type_ptr.hpp>

namespace Himii
{
    extern const std::filesystem::path s_AssetsPath;
    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> &context)
    {
        SetContext(context);
    }
    void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
    {
        m_Context = context;
    }
    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy");

        if (m_Context)
        {
            auto &view = m_Context->m_Registry.view<TagComponent>();
            for (auto entity: view)
            {
                Entity e{entity, m_Context.get()};
                DrawEntityNode(e);
            }
        }

        if (ImGui::BeginPopupContextWindow(0, 1))
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                m_Context->CreateEntity("Empty Entity");
            }
            ImGui::EndPopup();
        }
        ImGui::End();

        ImGui::Begin("Properties");

        if (m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto &tag = entity.GetComponent<TagComponent>().name;

        ImGuiTreeNodeFlags flags =
                ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        bool entityDeleted = false;
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entity"))
            {
                entityDeleted = true;
            }
            ImGui::EndPopup();
        }

        if (opened)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool opened = ImGui::TreeNodeEx((void *)9817239, flags, tag.c_str());
            if (opened)
                ImGui::TreePop();
            ImGui::TreePop();
        }

        if (entityDeleted)
        {
            m_Context->DestroyEntity(entity);
            if (m_SelectionContext == entity)
                m_SelectionContext = {};
        }
    }

    static void DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f,
                                float columnWidth = 100.0f)
    {
        ImGuiIO &io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        ImGui::PushID(label.c_str());

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text(label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.23f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("X", buttonSize))
            values.x = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.12f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Y", buttonSize))
            values.y = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Z
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.13f, 0.4f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushFont(boldFont);
        if (ImGui::Button("Z", buttonSize))
            values.z = resetValue;
        ImGui::PopFont();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string &name, Entity entity, UIFunction uiFunction)
    {
        const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                                 ImGuiTreeNodeFlags_SpanAvailWidth |
                                                 ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (entity.HasComponent<T>())
        {
            auto &component = entity.GetComponent<T>();
            ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{4, 4});
            float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImGui::Separator();
            bool opened = ImGui::TreeNodeEx((void *)typeid(T).hash_code(), treeNodeFlags, name.c_str());
            ImGui::PopStyleVar();

            ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
            if (ImGui::Button("+", ImVec2{lineHeight, lineHeight}))
            {
                ImGui::OpenPopup("ComponentSetting");
            }

            bool removeComponent = false;
            if (ImGui::BeginPopup("ComponentSetting"))
            {
                if (ImGui::MenuItem("Remove component"))
                    removeComponent = true;
                ImGui::EndPopup();
            }

            if (opened)
            {
                uiFunction(component);
                ImGui::TreePop();
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
        }
    }

    void SceneHierarchyPanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto &tag = entity.GetComponent<TagComponent>().name;

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));

            strcat(buffer, tag.c_str());
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }
        ImGui::SameLine();
        //ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<CameraComponent>("Camera");
            DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");

            ImGui::EndPopup();
        }

        DrawComponent<TransformComponent>("Transform", entity,
                                          [](auto &component)
                                          {
                                              DrawVec3Control("Position", component.Position);
                                              glm::vec3 rotation = glm::degrees(component.Rotation);
                                              DrawVec3Control("Rotation", rotation);
                                              component.Rotation = glm::radians(rotation);
                                              DrawVec3Control("Scale", component.Scale, 1.0f);
                                          });

        DrawComponent<CameraComponent>("Camera", entity,
                                       [](auto &component)
                                       {
                                           auto &camera = component.camera;

                                           ImGui::Checkbox("Primary",&component.primary);
                                           const char *projectionTypeStrings[] = {"Perspective", "Orthographic"};
                                           const char *currentProjectionTypeString =
                                                   projectionTypeStrings[(int)camera.GetProjectionType()];
                                           if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
                                           {
                                               for (int i = 0; i < 2; i++)
                                               {
                                                   bool isSelected =
                                                           currentProjectionTypeString == projectionTypeStrings[i];
                                                   if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                                                   {
                                                       currentProjectionTypeString = projectionTypeStrings[i];
                                                       camera.SetProjectionType((SceneCamera::ProjectionType)i);
                                                   }
                                                   if (isSelected)
                                                       ImGui::SetItemDefaultFocus();
                                               }
                                               ImGui::EndCombo();
                                           }
                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                                           {
                                               float perspectiveFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
                                               if (ImGui::DragFloat("Vertical FOV", &perspectiveFOV))
                                                   camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveFOV));

                                               float perspectiveNear = camera.GetPerspectiveNearClip();
                                               if (ImGui::DragFloat("Near", &perspectiveNear))
                                                   camera.SetPerspectiveNearClip(perspectiveNear);

                                               float perspectiveFar = camera.GetPerspectiveFarClip();
                                               if (ImGui::DragFloat("Far", &perspectiveFar))
                                                   camera.SetPerspectiveFarClip(perspectiveFar);
                                           }
                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                                           {
                                               float orthographicSize = camera.GetOrthographicSize();
                                               if (ImGui::DragFloat("Size", &orthographicSize))
                                                   camera.SetOrthographicSize(orthographicSize);

                                               float orthographicNear = camera.GetOrthographicNearClip();
                                               if (ImGui::DragFloat("Near", &orthographicNear))
                                                   camera.SetOrthographicNearClip(orthographicNear);

                                               float orthographicFar = camera.GetOrthographicFarClip();
                                               if (ImGui::DragFloat("Far", &orthographicFar))
                                                   camera.SetOrthographicFarClip(orthographicFar);

                                               ImGui::Checkbox("Fixed Aspect Ratio", &component.fixedAspectRatio);
                                           }
                                       });

        DrawComponent<SpriteRendererComponent>("Sprite Renderer", entity,
                                               [](auto &component)
                                               {
                                                   ImGui::ColorEdit4("Color", glm::value_ptr(component.color));
                                                   //Texture
                                                   ImGui::Button("Texture",ImVec2(100.0f,0.0f));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            const wchar_t *path = (const wchar_t *)payload->Data;
                            std::filesystem::path texturePath = std::filesystem::path(s_AssetsPath) / path;
                            component.texture = Texture2D::Create(texturePath.string());
                        }

                        ImGui::EndDragDropTarget();


                        ImGui::DragFloat("Tiling Factor", &component.tilingFactor, 0.1f, 0.0f, 100.0f);
                    }
                                               });
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

} // namespace Himii
