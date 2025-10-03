#pragma once
#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

namespace Himii
{
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

        auto view = m_Context->Registry().view<TransformComponent>();
        for ( auto e:view)
        {
            Entity entity{e, m_Context.get()};
            DrawEntityNode(entity);
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

        ImGuiTreeNodeFlags flags =( (m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }
        if (opened)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
            bool opened = ImGui::TreeNodeEx((void *)0, flags, tag.c_str());

            if (opened)
                ImGui::TreePop();
            ImGui::TreePop();
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
        if (entity.HasComponent<TransformComponent>())
        {
            if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
            {
                auto &transform = entity.GetComponent<TransformComponent>();
                ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.1f);

                ImGui::TreePop();
            }
        }
        if (entity.HasComponent<CameraComponent>())
        {
            if (ImGui::TreeNodeEx((void *)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen,
                                  "Camera"))
            {
                auto &cameraComponent = entity.GetComponent<CameraComponent>();
                auto &camera = cameraComponent.camera;

                ImGui::Checkbox("Primary", &cameraComponent.primary);

                const char *projectionTypeStrings[] = {"Perspective", "Orthographic"};
                const char *currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
                if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
                {
                    for (int i = 0; i < 2; i++)
                    {
                        bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
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

                    ImGui::Checkbox("Fixed Aspect Ratio", &cameraComponent.fixedAspectRatio);
                }
                ImGui::TreePop();
            }
        }
    }
} // namespace Himii
