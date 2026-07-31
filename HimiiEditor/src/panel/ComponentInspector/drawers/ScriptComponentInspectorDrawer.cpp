#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Project/Project.h"
#include "World/Scene/Components.h"
#include "Module/Script/ScriptEngine.h"
#include "Module/Script/ScriptIDELauncher.h"

#include <filesystem>

#include <imgui.h>

namespace Himii
{
    static void DrawScriptComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<ScriptComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<ScriptComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Script") : nullptr;
        const Ref<Scene> scene = drawContext.scene;
        const Entity entity = drawContext.entity;

        DrawComponentInspectorHeaderUI(
            drawContext, "ScriptComponent", "Script", icon,
            [&]()
            {
                DrawPropertyRow(
                    "Class Name",
                    [&]()
                    {
                        char classNameBuffer[256];
                        std::memset(classNameBuffer, 0, sizeof(classNameBuffer));
                        std::snprintf(classNameBuffer, sizeof(classNameBuffer), "%s", component.ClassName.c_str());

                        const bool scriptClassExistsForButton =
                            !component.ClassName.empty() && ScriptEngine::EntityClassExists(component.ClassName);
                        const float editButtonWidth =
                            ImGui::CalcTextSize("Edit").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                        const float reservedWidth = scriptClassExistsForButton
                            ? editButtonWidth + ImGui::GetStyle().ItemSpacing.x
                            : 0.0f;

                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - reservedWidth);
                        ImGui::InputText("##ClassName", classNameBuffer, sizeof(classNameBuffer));
                        if (ImGui::IsItemDeactivatedAfterEdit())
                            component.ClassName = classNameBuffer;

                        if (scriptClassExistsForButton)
                        {
                            ImGui::SameLine();
                            if (ImGui::Button("Edit", ImVec2(editButtonWidth, 0.0f)))
                            {
                                if (auto project = Project::GetActive())
                                {
                                    auto projectDirectory = Project::GetProjectDirectory();
                                    std::filesystem::path scriptPath =
                                        projectDirectory / "assets" / "scripts" / (component.ClassName + ".cs");

                                    if (std::filesystem::exists(scriptPath))
                                    {
                                        ScriptIDELauncher::OpenScript(
                                            projectDirectory, project->GetConfig().Name, scriptPath);
                                    }
                                }
                            }
                        }
                    });

                bool scriptClassExists = false;
                if (!component.ClassName.empty())
                    scriptClassExists = ScriptEngine::EntityClassExists(component.ClassName);

                if (!scriptClassExists && !component.ClassName.empty())
                    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "Invalid Script Class!");
                else if (scriptClassExists)
                {
                    auto& fields = ScriptEngine::GetScriptFieldMap(entity);

                    for (auto& [name, field] : fields)
                    {
                        if (field.Type == ScriptFieldType::Float)
                        {
                            float data = field.GetValue<float>();
                            float oldData = data;
                            DrawFloatControl(name, data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetFloat(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::Int)
                        {
                            int data = field.GetValue<int>();
                            int oldData = data;
                            DrawIntControl(name.c_str(), data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetInt(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::Bool)
                        {
                            bool data = field.GetValue<bool>();
                            bool oldData = data;
                            DrawCheckboxControl(name, data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetBool(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::Vector2)
                        {
                            glm::vec2 data = field.GetValue<glm::vec2>();
                            glm::vec2 oldData = data;
                            DrawVec2Control(name.c_str(), data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetVector2(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::Vector3)
                        {
                            glm::vec3 data = field.GetValue<glm::vec3>();
                            glm::vec3 oldData = data;
                            DrawVec3Control(name, data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetVector3(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::Vector4)
                        {
                            glm::vec4 data = field.GetValue<glm::vec4>();
                            glm::vec4 oldData = data;
                            DrawColorControl(name, data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetVector4(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::String)
                        {
                            std::string data = field.GetValue<std::string>();
                            std::string oldData = data;
                            DrawStdStringControl(name.c_str(), data);
                            if (data != oldData)
                            {
                                field.SetValue(data);
                                ScriptEngine::SetString(
                                    ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                            }
                        }
                        else if (field.Type == ScriptFieldType::KeyCode)
                        {
                            int data = field.GetValue<int>();

                            struct KeyCodeOption
                            {
                                int Code;
                                const char* Label;
                            };
                            static const KeyCodeOption keyCodeOptions[] = {
                                {32, "Space"},
                                {65, "A"},
                                {68, "D"},
                                {83, "S"},
                                {87, "W"},
                                {262, "Right"},
                                {263, "Left"},
                                {264, "Down"},
                                {265, "Up"},
                                {257, "Enter"},
                            };

                            const char* keyCodeLabels[std::size(keyCodeOptions)] = {};
                            for (size_t optionIndex = 0; optionIndex < std::size(keyCodeOptions); ++optionIndex)
                                keyCodeLabels[optionIndex] = keyCodeOptions[optionIndex].Label;

                            int selectedOptionIndex = 0;
                            for (size_t optionIndex = 0; optionIndex < std::size(keyCodeOptions); ++optionIndex)
                            {
                                if (keyCodeOptions[optionIndex].Code == data)
                                {
                                    selectedOptionIndex = static_cast<int>(optionIndex);
                                    break;
                                }
                            }

                            DrawEnumComboControl(
                                name.c_str(), selectedOptionIndex, keyCodeLabels,
                                static_cast<int>(std::size(keyCodeOptions)),
                                [&](int newIndex)
                                {
                                    data = keyCodeOptions[newIndex].Code;
                                    field.SetValue(data);
                                    ScriptEngine::SetInt(
                                        ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                });
                        }
                        else if (field.Type == ScriptFieldType::Entity)
                        {
                            UUID data = field.GetValue<UUID>();
                            DrawPropertyRow(
                                name.c_str(),
                                [&]()
                                {
                                    std::string preview = "None";
                                    if (data)
                                    {
                                        Entity referenceEntity = scene->GetEntityByUUID(data);
                                        if (referenceEntity)
                                            preview = referenceEntity.GetName();
                                        else
                                            preview = "Missing";
                                    }

                                    ImGui::PushItemWidth(-1.0f);
                                    if (ImGui::BeginCombo("##EntityReference", preview.c_str()))
                                    {
                                        if (ImGui::Selectable("None", data == UUID{}))
                                        {
                                            data = {};
                                            field.SetValue(data);
                                            ScriptEngine::SetEntityField(
                                                ScriptEngine::GetEntityScriptInstance(entity.GetUUID()), name, data);
                                        }

                                        if (scene)
                                        {
                                            const auto& view = scene->Registry().view<TagComponent>();
                                            for (auto sceneHandle : view)
                                            {
                                                Entity sceneEntity{sceneHandle, scene.get()};
                                                if (sceneEntity == entity)
                                                    continue;

                                                UUID sceneUUID = sceneEntity.GetUUID();
                                                const bool isSelected = sceneUUID == data;
                                                if (ImGui::Selectable(sceneEntity.GetName().c_str(), isSelected))
                                                {
                                                    data = sceneUUID;
                                                    field.SetValue(data);
                                                    ScriptEngine::SetEntityField(
                                                        ScriptEngine::GetEntityScriptInstance(entity.GetUUID()),
                                                        name, data);
                                                }
                                                if (isSelected)
                                                    ImGui::SetItemDefaultFocus();
                                            }
                                        }
                                        ImGui::EndCombo();
                                    }
                                    ImGui::PopItemWidth();
                                });
                        }
                    }
                }
            },
            [&]() { drawContext.entity.RemoveComponent<ScriptComponent>(); });
    }

    struct ScriptComponentInspectorRegistrar
    {
        ScriptComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<ScriptComponent>(
                "ScriptComponent", "Script", "Script", 20, &DrawScriptComponentInspectorUI);
        }
    };

    static ScriptComponentInspectorRegistrar s_ScriptComponentInspectorRegistrar;
}
