#pragma once
#include "SceneHierarchyPanel.h"
#include "Himii/Project/Project.h"

#include "Himii/Scripting/ScriptEngine.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include "filesystem"

#include <glm/gtc/type_ptr.hpp>

namespace Himii
{
    extern const std::filesystem::path s_AssetsPath;
    SceneHierarchyPanel::SceneHierarchyPanel()
    {
        // Load Component Icons
        m_ComponentIcons["Transform"] = Texture2D::Create("resources/icons/Component_Transform.png");
        m_ComponentIcons["Camera"] = Texture2D::Create("resources/icons/Component_Camera.png");
        m_ComponentIcons["Script"] = Texture2D::Create("resources/icons/Component_Script.png");
        m_ComponentIcons["Sprite Renderer"] = Texture2D::Create("resources/icons/Component_SpriteRenderer.png");
        m_ComponentIcons["Circle Renderer"] = Texture2D::Create("resources/icons/Component_CircleRenderer.png");
        m_ComponentIcons["Rigidbody2D"] = Texture2D::Create("resources/icons/Component_Rigidbody.png");
        m_ComponentIcons["Box Collider2D"] = Texture2D::Create("resources/icons/Component_BoxCollider.png");
        m_ComponentIcons["Circle Collider2D"] = Texture2D::Create("resources/icons/Component_CircleCollider.png");
        m_ComponentIcons["Sprite Animation"] = Texture2D::Create("resources/icons/Component_Animator.png");
    }

    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
        : SceneHierarchyPanel()
    {
        SetContext(context);
    }
    void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
    {
        m_Context = context;
    }
    void SceneHierarchyPanel::OnImGuiRender()
    {
        ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);

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

        ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoCollapse);

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
                DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider2D");

                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
    {
        auto &tag = entity.GetComponent<TagComponent>().Tag;

        ImGuiTreeNodeFlags flags =
                ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        flags |= ImGuiTreeNodeFlags_Leaf; // Fix: Treat as leaf node preventing arrow
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
        
        // Use true for border to create the separator
        if (ImGui::BeginTable(label.c_str(), 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
             ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
             ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
             // ImGui::TableHeadersRow();

             ImGui::TableNextColumn();
             ImGui::Text("%s", label.c_str());
             ImGui::TableNextColumn();


             ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

             float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
             ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
             float widthEach = (ImGui::GetContentRegionAvail().x - 3 * buttonSize.x) / 3.0f;

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
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
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
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
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
             ImGui::SetNextItemWidth(widthEach);
             ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");

             ImGui::PopStyleVar();

             ImGui::EndTable();
        }

        ImGui::PopID();
    }

    static void DrawFloatControl(const std::string& label, float& value, float speed = 0.1f, float min = 0.0f, float max = 0.0f, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##FloatControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::DragFloat("##Value", &value, speed, min, max);
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    static void DrawCheckboxControl(const std::string& label, bool& value, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##CheckboxControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::Checkbox("##Value", &value);
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    static void DrawColorControl(const std::string& label, glm::vec4& value, float columnWidth = 100.0f)
    {
        ImGui::PushID(label.c_str());
        if(ImGui::BeginTable("##ColorControl", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextColumn();
            ImGui::Text("%s", label.c_str());
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::ColorEdit4("##Value", glm::value_ptr(value));
            ImGui::PopItemWidth();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    template<typename T, typename UIFunction>
    static void DrawComponent(const std::string &name, Entity entity, Ref<Texture2D> icon, UIFunction uiFunction)
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
            
        // --- Custom Icon Header ---
            void* componentID = (void*)typeid(T).hash_code();
            ImGui::PushID(componentID);
            
            bool open = ImGui::GetStateStorage()->GetInt(ImGui::GetID("IsOpen"), 1);

            // Calculate start pos
            ImVec2 cursorPos = ImGui::GetCursorScreenPos(); // Screen Space
            ImVec2 elementPos = ImGui::GetCursorPos();      // Window Space (for restoring)
            
            // 1. Draw Background (Lighter color)
            ImU32 headerColor = ImGui::GetColorU32(ImVec4{0.27f, 0.275f, 0.28f, 1.0f}); // Brighter gray
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursorPos, 
                ImVec2(cursorPos.x + contentRegionAvailable.x, cursorPos.y + lineHeight), 
                headerColor
            );

            // 2. Draw Selectable (Input handling)
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0,0,0,0)); 
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0,0,0,0)); 
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0)); 

            if (ImGui::Selectable("##Header", false, ImGuiSelectableFlags_AllowItemOverlap, ImVec2(contentRegionAvailable.x, lineHeight)))
            {
                open = !open;
                ImGui::GetStateStorage()->SetInt(ImGui::GetID("IsOpen"), open ? 1 : 0);
            }
            ImGui::PopStyleColor(3);

            // 3. Draw Arrow & Icon & Text (Overlay)
            // Reset cursor to start of the item to draw on top
            ImGui::SetCursorPos(ImVec2(elementPos.x + ImGui::GetStyle().WindowPadding.x, elementPos.y));

            // Arrow
            ImGui::RenderArrow(ImGui::GetWindowDrawList(), 
                ImVec2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + (lineHeight - GImGui->FontSize)/2), 
                ImGui::GetColorU32(ImGuiCol_Text), 
                open ? ImGuiDir_Down : ImGuiDir_Right
            );
            
            // Move cursor past arrow
            ImGui::SetCursorPos(ImVec2(elementPos.x + ImGui::GetStyle().WindowPadding.x + GImGui->FontSize + 4.0f, elementPos.y));

            // Icon
            if (icon)
            {
                ImGui::Image((ImTextureID)icon->GetRendererID(), ImVec2{lineHeight - 4, lineHeight - 4}, {0, 1}, {1, 0});
                ImGui::SameLine();
            }
            
            // Text alignment fix (center vertically roughly)
            float textOffsetY = (lineHeight - GImGui->FontSize) / 2.0f;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffsetY);
            ImGui::TextUnformatted(name.c_str());
            
            // Restore proper cursor Y for next item (although Selectable already advanced it, we messed with it)
            // Selectable advanced Y by lineHeight. 
            // Our overlay drawing logic mostly stayed on the same line or advanced locally.
            // We should ensure we are at the line *after* the header.
            ImGui::SetCursorPos(ImVec2(elementPos.x, elementPos.y + lineHeight));

            ImGui::PopID();
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

            if (open)
            {
                uiFunction(component);
            }

            if (removeComponent)
                entity.RemoveComponent<T>();
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
            if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            {
                tag = std::string(buffer);
            }
        }
        ImGui::SameLine();
        // ImGui::PushItemWidth(-1);
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponent");

        if (ImGui::BeginPopup("AddComponent"))
        {
            DisplayAddComponentEntry<CameraComponent>("Camera");
            DisplayAddComponentEntry<ScriptComponent>("Script Component");
            DisplayAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
            DisplayAddComponentEntry<CircleRendererComponent>("Circle Renderer");
            DisplayAddComponentEntry<Rigidbody2DComponent>("Rigidbody2D");
            DisplayAddComponentEntry<BoxCollider2DComponent>("Box Collider2D");
            DisplayAddComponentEntry<CircleCollider2DComponent>("Circle Collider2D");
            DisplayAddComponentEntry<SpriteAnimationComponent>("Sprite Animation");

            ImGui::EndPopup();
        }

        DrawComponent<TransformComponent>("Transform", entity, m_ComponentIcons["Transform"],
                                          [](auto &component)
                                          {
                                              DrawVec3Control("Position", component.Position);
                                              glm::vec3 rotation = glm::degrees(component.Rotation);
                                              DrawVec3Control("Rotation", rotation);
                                              component.Rotation = glm::radians(rotation);
                                              DrawVec3Control("Scale", component.Scale, 1.0f);
                                          });

        DrawComponent<CameraComponent>("Camera", entity, m_ComponentIcons["Camera"],
                                       [](auto &component)
                                       {
                                           auto &camera = component.Camera;
                                           
                                           glm::vec4 backgroundColor = camera.GetBackgroundColor();
                                           if (ImGui::ColorEdit4("Background Color", glm::value_ptr(backgroundColor)))
                                           {
                                               camera.SetBackgroundColor(backgroundColor);
                                           }

                                           DrawCheckboxControl("Primary", component.Primary);

                                           const char *projectionTypeStrings[] = {"Perspective", "Orthographic"};
                                           const char *currentProjectionTypeString =
                                                   projectionTypeStrings[(int)camera.GetProjectionType()];
                                           
                                           ImGui::PushID("Projection");
                                           if(ImGui::BeginTable("##Projection", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                                           {
                                                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                                                ImGui::TableNextColumn();
                                                ImGui::Text("Projection");
                                                ImGui::TableNextColumn();
                                                ImGui::PushItemWidth(-1);
                                                if (ImGui::BeginCombo("##Projection", currentProjectionTypeString))
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
                                                ImGui::PopItemWidth();
                                                ImGui::EndTable();
                                           }
                                           ImGui::PopID();

                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                                           {
                                               float perspectiveFOV = glm::degrees(camera.GetPerspectiveVerticalFOV());
                                               DrawFloatControl("Vertical FOV", perspectiveFOV);
                                               if (perspectiveFOV != glm::degrees(camera.GetPerspectiveVerticalFOV()))
                                                   camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveFOV));

                                               float perspectiveNear = camera.GetPerspectiveNearClip();
                                               DrawFloatControl("Near", perspectiveNear);
                                               camera.SetPerspectiveNearClip(perspectiveNear);

                                               float perspectiveFar = camera.GetPerspectiveFarClip();
                                               DrawFloatControl("Far", perspectiveFar);
                                               camera.SetPerspectiveFarClip(perspectiveFar);
                                           }
                                           if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                                           {
                                               float orthographicSize = camera.GetOrthographicSize();
                                               DrawFloatControl("Size", orthographicSize);
                                               camera.SetOrthographicSize(orthographicSize);

                                               float orthographicNear = camera.GetOrthographicNearClip();
                                               DrawFloatControl("Near", orthographicNear);
                                               camera.SetOrthographicNearClip(orthographicNear);

                                               float orthographicFar = camera.GetOrthographicFarClip();
                                               DrawFloatControl("Far", orthographicFar);
                                               camera.SetOrthographicFarClip(orthographicFar);

                                               DrawCheckboxControl("Fixed Aspect Ratio", component.FixedAspectRatio);
                                           }
                                       });

        #include <cstdlib> // for system()

        // ...

        DrawComponent<ScriptComponent>(
                "Script", entity, m_ComponentIcons["Script"],
                [entity, scene = m_Context](auto &component) mutable
                {
                    bool scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

                    ImGui::Text("Class Name");
                    ImGui::SameLine();

                    if (ImGui::InputText("##ClassName", &component.ClassName))
                    {
                        // 输入修改逻辑
                    }

                    if (!scriptClassExists)
                    {
                        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Invalid Script Class!");
                    }
                    else
                    {
                        // --- 新增：打开 IDE 逻辑 ---
                        ImGui::SameLine();
                        if (ImGui::Button("Edit"))
                        {
                            // 1. 获取当前激活的项目
                            if (auto project = Project::GetActive())
                            {
                                auto projectDir = Project::GetProjectDirectory();
                                // ... (文件名解析代码) ...
                                std::filesystem::path scriptPath =
                                        projectDir / "assets" / "scripts" / (component.ClassName + ".cs");

                                if (std::filesystem::exists(scriptPath))
                                {
                                    // VS Code 技巧：
                                    // "code folder file" 会打开文件夹并在其中打开文件

                                    // 组合命令：code "E:/Project/Sandbox" "E:/Project/Sandbox/assets/scripts/Player.cs"
                                    std::string cmd =
                                            "code \"" + projectDir.string() + "\" \"" + scriptPath.string() + "\"";
                                    system(cmd.c_str());
                                }
                            }
                        }
                    }
                });

        DrawComponent<SpriteRendererComponent>(
                "Sprite Renderer", entity, m_ComponentIcons["Sprite Renderer"],
                [](auto &component)
                {
                    DrawColorControl("Color", component.Color);
                    // Texture
                    ImGui::PushID("Texture");
                    if(ImGui::BeginTable("##Texture", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Texture");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::Button("Texture", ImVec2(100.0f, 0.0f));
                        ImGui::PopItemWidth();
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                            {
                                const wchar_t *path = (const wchar_t *)payload->Data;
                                std::filesystem::path texturePath = Project::GetAssetDirectory() / path;

                                if (std::filesystem::exists(texturePath))
                                    component.Texture = Texture2D::Create(texturePath.string());
                                else
                                    HIMII_CORE_WARNING("Texture not found: {0}", texturePath.string());
                            }

                            ImGui::EndDragDropTarget();
                        }
                        ImGui::EndTable();
                    }
                    ImGui::PopID();

                    DrawFloatControl("Tiling Factor", component.TilingFactor, 0.1f, 0.0f, 100.0f);
                });

        DrawComponent<CircleRendererComponent>("Circle Renderer", entity, m_ComponentIcons["Circle Renderer"],
                                               [](auto &component)
                                               {
                                                   DrawColorControl("Color", component.Color);
                                                   DrawFloatControl("Thickness", component.Thickness, 0.025f, 0.0f, 1.0f);
                                                   DrawFloatControl("Fade", component.Fade, 0.0003f, 0.0f, 1.0f);
                                               });

        DrawComponent<Rigidbody2DComponent>("Rigidbody2D", entity, m_ComponentIcons["Rigidbody2D"],
                                            [](auto &component)
                                            {
                                                ImGui::PushID("Body Type");
                                                if(ImGui::BeginTable("##BodyType", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingFixedFit))
                                                {
                                                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                                                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                                                    ImGui::TableNextColumn();

                                                    ImGui::Text("Body Type");
                                                    ImGui::TableNextColumn();
                                                    ImGui::PushItemWidth(-1);
                                                    const char *bodyTypeStrings[] = {"Static", "Dynamic", "Kinematic"};
                                                    const char *currentBodyTypeString = bodyTypeStrings[(int)component.Type];
                                                    if (ImGui::BeginCombo("##Body Type", currentBodyTypeString))
                                                    {
                                                        for (int i = 0; i < 3; i++)
                                                        {
                                                            bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
                                                            if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
                                                            {
                                                                currentBodyTypeString = bodyTypeStrings[i];
                                                                component.Type = (Rigidbody2DComponent::BodyType)i;
                                                            }
                                                            if (isSelected) ImGui::SetItemDefaultFocus();
                                                        }
                                                        ImGui::EndCombo();
                                                    }
                                                    ImGui::PopItemWidth();
                                                    ImGui::EndTable();
                                                }
                                                ImGui::PopID();

                                                DrawCheckboxControl("Fixed Rotation", component.FixedRotation);
                                            });

        DrawComponent<BoxCollider2DComponent>(
                "Box Collider2D", entity, m_ComponentIcons["Box Collider2D"],
                [](auto &component)
                {
                    // For vec2, we can just use DrawFloatControl twice or keep usage? 
                    // Let's implement vec2 logic or simpler:
                    ImGui::PushID("Offset");
                    if(ImGui::BeginTable("##Offset", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Offset");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    ImGui::PushID("Size");
                    if(ImGui::BeginTable("##Size", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Size");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Size", glm::value_ptr(component.Size), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    DrawFloatControl("Density", component.Density, 0.1f);
                    DrawFloatControl("Friction", component.Friction, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution", component.Restitution, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution Threshold", component.RestitutionThreshold, 0.1f);
                });
        DrawComponent<CircleCollider2DComponent>(
                "Circle Collider2D", entity, m_ComponentIcons["Circle Collider2D"],
                [](auto &component)
                {
                    ImGui::PushID("Offset");
                    if(ImGui::BeginTable("##Offset", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextColumn();
                        ImGui::Text("Offset");
                        ImGui::TableNextColumn();
                        ImGui::PushItemWidth(-1);
                        ImGui::DragFloat2("##Offset", glm::value_ptr(component.Offset), 0.1f);
                        ImGui::PopItemWidth();
                        ImGui::EndTable();
                    }
                    ImGui::PopID();
                    
                    DrawFloatControl("Radius", component.Radius, 0.1f);
                    DrawFloatControl("Density", component.Density, 0.1f);
                    DrawFloatControl("Friction", component.Friction, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution", component.Restitution, 0.01f, 0.0f, 1.0f);
                    DrawFloatControl("Restitution Threshold", component.RestitutionThreshold, 0.1f);
                });
        DrawComponent<SpriteAnimationComponent>(
                "Sprite Animation", entity, m_ComponentIcons["Sprite Animation"],
                [](auto &component)
                {
                    ImGui::Text("Asset Handle: %llu", (uint64_t)component.AnimationHandle);

                    ImGui::Button("Animation Asset", ImVec2(100.0f, 0.0f));
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            const wchar_t *path = (const wchar_t *)payload->Data;
                            std::filesystem::path assetPath = path;

                            if (assetPath.extension() == ".anim")
                            {
                                // 获取 AssetManager (需要 #include "Himii/Project/Project.h")
                                auto assetManager = Project::GetAssetManager();
                                if (assetManager)
                                {
                                    AssetHandle handle = assetManager->ImportAsset(assetPath);
                                    if (handle != 0)
                                        component.AnimationHandle = handle;
                                }
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::DragFloat("Frame Rate", &component.FrameRate, 0.1f, 0.0f, 60.0f);
                    ImGui::Checkbox("Playing", &component.Playing);
                    ImGui::Text("Current Frame: %d", component.CurrentFrame);
                    ImGui::Text("Timer: %.2f", component.Timer);
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
