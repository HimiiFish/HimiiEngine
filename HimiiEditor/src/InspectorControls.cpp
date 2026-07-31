#include "Hepch.h"
#include "InspectorControls.h"

#include "Resource/AssetManager.h"
#include "Resource/SpriteSheetUtility.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace Himii
{

    namespace
    {
        constexpr float InspectorFloatResetEpsilon = 0.0001f;

        bool IsFloatNear(float left, float right)
        {
            return std::fabs(left - right) <= InspectorFloatResetEpsilon;
        }

        bool IsVec2Near(const glm::vec2& left, const glm::vec2& right)
        {
            return IsFloatNear(left.x, right.x) && IsFloatNear(left.y, right.y);
        }

        bool IsVec3Near(const glm::vec3& left, const glm::vec3& right)
        {
            return IsFloatNear(left.x, right.x) && IsFloatNear(left.y, right.y)
                   && IsFloatNear(left.z, right.z);
        }

        bool IsVec4Near(const glm::vec4& left, const glm::vec4& right)
        {
            return IsFloatNear(left.x, right.x) && IsFloatNear(left.y, right.y)
                   && IsFloatNear(left.z, right.z) && IsFloatNear(left.w, right.w);
        }
    }

    void BeginInspectorPropertiesStyle()
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                            ImVec2(style.ItemSpacing.x, style.ItemSpacing.y * 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                            ImVec2(style.FramePadding.x, style.FramePadding.y * 0.55f));
    }

    void EndInspectorPropertiesStyle()
    {
        ImGui::PopStyleVar(2);
    }

    void DrawInspectorTooltipIfHovered(const char* tooltipText)
    {
        if (!tooltipText || tooltipText[0] == '\0')
            return;

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("%s", tooltipText);
    }

    void DrawPropertyRow(const char* label, const std::function<void()>& drawValueColumn,
                         const char* tooltipText, bool showResetButton,
                         const std::function<void()>& onReset)
    {
        ImGui::PushID(label);
        if (ImGui::BeginTable("##PropertyRow", 3,
                              ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, InspectorLabelColumnWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, InspectorResetColumnWidth);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();
            drawValueColumn();
            ImGui::TableNextColumn();
            if (showResetButton && onReset)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
                if (ImGui::SmallButton("↺"))
                    onReset();
                ImGui::PopStyleVar();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("Reset");
            }
            ImGui::EndTable();
        }
        DrawInspectorTooltipIfHovered(tooltipText);
        ImGui::PopID();
    }

    void DrawFloatControl(const std::string& label, float& value, float speed, float minimum, float maximum,
                          const std::function<void()>& onEditBegin,
                          const std::function<void()>& onEditEnd, bool enableRowReset, float resetValue)
    {
        const bool showReset = enableRowReset && !IsFloatNear(value, resetValue);
        DrawPropertyRow(
            label.c_str(),
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                ImGui::DragFloat("##Value", &value, speed, minimum, maximum);
                ImGui::PopItemWidth();
                if (ImGui::IsItemActivated() && onEditBegin)
                    onEditBegin();
                if (ImGui::IsItemDeactivatedAfterEdit() && onEditEnd)
                    onEditEnd();
            },
            nullptr, showReset,
            [&]()
            {
                value = resetValue;
                if (onEditEnd)
                    onEditEnd();
            });
    }

    void DrawIntControl(const char* label, int& value, float speed, int minimum, int maximum,
                        bool enableRowReset, int resetValue)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label,
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                if (minimum != 0 || maximum != 0)
                    ImGui::DragInt("##Value", &value, speed, minimum, maximum);
                else
                    ImGui::DragInt("##Value", &value, speed);
                ImGui::PopItemWidth();
            },
            nullptr, showReset, [&]() { value = resetValue; });
    }

    void DrawColorControl(const std::string& label, glm::vec4& value, const glm::vec4& resetValue)
    {
        const bool showReset = !IsVec4Near(value, resetValue);
        DrawPropertyRow(
            label.c_str(),
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                ImGui::ColorEdit4("##Value", glm::value_ptr(value));
                ImGui::PopItemWidth();
            },
            nullptr, showReset, [&]() { value = resetValue; });
    }

    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue,
                         const std::function<void()>& onEditBegin,
                         const std::function<void()>& onEditEnd, bool enableRowReset)
    {
        const glm::vec3 defaultValues(resetValue);
        const bool showReset = enableRowReset && !IsVec3Near(values, defaultValues);
        ImFont* boldFont = ImGui::GetIO().Fonts->Fonts[0];

        DrawPropertyRow(
            label.c_str(),
            [&]()
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
                const float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
                const float widthEach =
                    (ImGui::GetContentRegionAvail().x - 3.0f * buttonSize.x) / 3.0f;

                auto drawAxis = [&](const char* axisLabel, float& axisValue, const ImVec4& color,
                                    const ImVec4& hoveredColor, const ImVec4& activeColor)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                    ImGui::PushFont(boldFont);
                    if (ImGui::Button(axisLabel, buttonSize))
                    {
                        if (onEditBegin)
                            onEditBegin();
                        axisValue = resetValue;
                        if (onEditEnd)
                            onEditEnd();
                    }
                    ImGui::PopFont();
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(widthEach);
                    std::string inputIdentifier = std::string("##") + axisLabel;
                    ImGui::DragFloat(inputIdentifier.c_str(), &axisValue, 0.1f, 0.0f, 0.0f, "%.2f");
                    if (ImGui::IsItemActivated() && onEditBegin)
                        onEditBegin();
                    if (ImGui::IsItemDeactivatedAfterEdit() && onEditEnd)
                        onEditEnd();
                };

                drawAxis("X", values.x, ImVec4{0.8f, 0.23f, 0.12f, 1.0f},
                         ImVec4{0.9f, 0.2f, 0.2f, 1.0f}, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
                ImGui::SameLine();
                drawAxis("Y", values.y, ImVec4{0.12f, 0.7f, 0.2f, 1.0f},
                         ImVec4{0.3f, 0.8f, 0.3f, 1.0f}, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
                ImGui::SameLine();
                drawAxis("Z", values.z, ImVec4{0.13f, 0.4f, 0.8f, 1.0f},
                         ImVec4{0.2f, 0.35f, 0.9f, 1.0f}, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
                ImGui::PopStyleVar();
            },
            nullptr, showReset,
            [&]()
            {
                values = defaultValues;
                if (onEditEnd)
                    onEditEnd();
            });
    }

    void DrawVec2AxisControl(const std::string& label, glm::vec2& values, float resetValue,
                             const std::function<void()>& onEditBegin,
                             const std::function<void()>& onEditEnd, bool enableRowReset)
    {
        const glm::vec2 defaultValues(resetValue);
        const bool showReset = enableRowReset && !IsVec2Near(values, defaultValues);
        ImFont* boldFont = ImGui::GetIO().Fonts->Fonts[0];

        DrawPropertyRow(
            label.c_str(),
            [&]()
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, 0.0f});
                const float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
                const ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};
                const float widthEach =
                    (ImGui::GetContentRegionAvail().x - 2.0f * buttonSize.x) / 2.0f;

                auto drawAxis = [&](const char* axisLabel, float& axisValue, const ImVec4& color,
                                    const ImVec4& hoveredColor, const ImVec4& activeColor)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, color);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                    ImGui::PushFont(boldFont);
                    if (ImGui::Button(axisLabel, buttonSize))
                    {
                        if (onEditBegin)
                            onEditBegin();
                        axisValue = resetValue;
                        if (onEditEnd)
                            onEditEnd();
                    }
                    ImGui::PopFont();
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(widthEach);
                    std::string inputIdentifier = std::string("##") + axisLabel;
                    ImGui::DragFloat(inputIdentifier.c_str(), &axisValue, 0.1f, 0.0f, 0.0f, "%.2f");
                    if (ImGui::IsItemActivated() && onEditBegin)
                        onEditBegin();
                    if (ImGui::IsItemDeactivatedAfterEdit() && onEditEnd)
                        onEditEnd();
                };

                drawAxis("X", values.x, ImVec4{0.8f, 0.23f, 0.12f, 1.0f},
                         ImVec4{0.9f, 0.2f, 0.2f, 1.0f}, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
                ImGui::SameLine();
                drawAxis("Y", values.y, ImVec4{0.12f, 0.7f, 0.2f, 1.0f},
                         ImVec4{0.3f, 0.8f, 0.3f, 1.0f}, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
                ImGui::PopStyleVar();
            },
            nullptr, showReset,
            [&]()
            {
                values = defaultValues;
                if (onEditEnd)
                    onEditEnd();
            });
    }

    void DrawStdStringControl(const char* label, std::string& value,
                              const std::function<void()>& onEdited, bool enableRowReset,
                              const std::string& resetValue)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label,
            [&]()
            {
                char buffer[256];
                std::memset(buffer, 0, sizeof(buffer));
                std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
                ImGui::PushItemWidth(-1.0f);
                ImGui::InputText("##Value", buffer, sizeof(buffer));
                ImGui::PopItemWidth();
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    value = buffer;
                    if (onEdited)
                        onEdited();
                }
            },
            nullptr, showReset,
            [&]()
            {
                value = resetValue;
                if (onEdited)
                    onEdited();
            });
    }

    void DrawCheckboxControl(const std::string& label, bool& value, bool resetValue,
                             bool enableRowReset)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label.c_str(), [&]() { ImGui::Checkbox("##Value", &value); }, nullptr, showReset,
            [&]() { value = resetValue; });
    }

    void DrawEnumComboControl(const char* label, int& currentIndex, const char* const* labels,
                              int labelCount, const std::function<void(int newIndex)>& onSelectionChanged,
                              bool enableRowReset, int resetIndex)
    {
        const bool showReset =
            enableRowReset && currentIndex != resetIndex && resetIndex >= 0 && resetIndex < labelCount;
        DrawPropertyRow(
            label,
            [&]()
            {
                const char* preview = (currentIndex >= 0 && currentIndex < labelCount)
                                          ? labels[currentIndex]
                                          : "None";
                ImGui::PushItemWidth(-1.0f);
                if (ImGui::BeginCombo("##EnumCombo", preview))
                {
                    for (int index = 0; index < labelCount; ++index)
                    {
                        const bool isSelected = index == currentIndex;
                        if (ImGui::Selectable(labels[index], isSelected))
                        {
                            currentIndex = index;
                            onSelectionChanged(index);
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            },
            nullptr, showReset,
            [&]()
            {
                currentIndex = resetIndex;
                onSelectionChanged(resetIndex);
            });
    }

    void DrawUInt32Control(const char* label, uint32_t& value, float speed, bool enableRowReset,
                           uint32_t resetValue)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label,
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                ImGui::DragScalar("##Value", ImGuiDataType_U32, &value, speed, nullptr, nullptr, "%u");
                ImGui::PopItemWidth();
            },
            nullptr, showReset, [&]() { value = resetValue; });
    }

    void DrawIVec2Control(const char* label, glm::ivec2& value, float speed, int minimum, int maximum,
                          bool enableRowReset, glm::ivec2 resetValue)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label,
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                if (minimum != 0 || maximum != 0)
                    ImGui::DragInt2("##Value", &value.x, speed, minimum, maximum);
                else
                    ImGui::DragInt2("##Value", &value.x, speed);
                ImGui::PopItemWidth();
            },
            nullptr, showReset, [&]() { value = resetValue; });
    }

    void DrawVec2Control(const char* label, glm::vec2& value, float speed, float minimum, float maximum,
                         const std::function<void()>& onEdited, bool enableRowReset, glm::vec2 resetValue)
    {
        const bool showReset = enableRowReset && !IsVec2Near(value, resetValue);
        DrawPropertyRow(
            label,
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                if (minimum != 0.0f || maximum != 0.0f)
                    ImGui::DragFloat2("##Value", &value.x, speed, minimum, maximum);
                else
                    ImGui::DragFloat2("##Value", &value.x, speed);
                ImGui::PopItemWidth();
                if (onEdited && ImGui::IsItemDeactivatedAfterEdit())
                    onEdited();
            },
            nullptr, showReset,
            [&]()
            {
                value = resetValue;
                if (onEdited)
                    onEdited();
            });
    }

    void DrawIVec4Control(const char* label, glm::ivec4& value, float speed, int minimum, int maximum,
                          const std::function<void()>& onEdited, bool enableRowReset, glm::ivec4 resetValue)
    {
        const bool showReset = enableRowReset && value != resetValue;
        DrawPropertyRow(
            label,
            [&]()
            {
                ImGui::PushItemWidth(-1.0f);
                ImGui::DragInt4("##Value", &value.x, speed, minimum, maximum);
                ImGui::PopItemWidth();
                if (onEdited && ImGui::IsItemDeactivatedAfterEdit())
                    onEdited();
            },
            nullptr, showReset,
            [&]()
            {
                value = resetValue;
                if (onEdited)
                    onEdited();
            });
    }

    void DrawInputTextControl(const char* label, char* buffer, int bufferSize,
                              const std::function<void()>& onEdited)
    {
        DrawPropertyRow(label, [&]()
        {
            ImGui::PushItemWidth(-1.0f);
            ImGui::InputText("##Value", buffer, static_cast<size_t>(bufferSize));
            ImGui::PopItemWidth();
            if (onEdited && ImGui::IsItemDeactivatedAfterEdit())
                onEdited();
        });
    }

    void DrawMultilineTextControl(const char* label, std::string& value, int lineCount,
                                  const std::function<void()>& onEdited)
    {
        if (lineCount < 1)
            lineCount = 1;

        DrawPropertyRow(label, [&]()
        {
            char buffer[2048];
            std::memset(buffer, 0, sizeof(buffer));
            std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());

            const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
            const ImVec2 inputSize(-1.0f, lineHeight * static_cast<float>(lineCount));

            ImGui::PushItemWidth(-1.0f);
            const bool edited = ImGui::InputTextMultiline("##MultilineValue", buffer, sizeof(buffer), inputSize);
            ImGui::PopItemWidth();

            if (edited || ImGui::IsItemDeactivatedAfterEdit())
            {
                value = buffer;
                if (onEdited && ImGui::IsItemDeactivatedAfterEdit())
                    onEdited();
            }
        });
    }

    void DrawReadOnlyTextControl(const char* label, const char* text, const char* tooltipText)
    {
        DrawPropertyRow(label, [&]()
        {
            if (text && text[0] != '\0')
                ImGui::TextUnformatted(text);
            else
                ImGui::TextDisabled("None");
        }, tooltipText);
    }

    void DrawInspectorSectionHeader(const char* title, const char* tooltipText)
    {
        ImGui::Spacing();
        ImGui::SeparatorText(title);
        DrawInspectorTooltipIfHovered(tooltipText);
    }

    void DrawActionButtonRow(const char* label, const std::function<void()>& drawButtons)
    {
        DrawPropertyRow(label, drawButtons);
    }

    bool DrawEditorToggleButton(const char* label, bool isActive, const char* tooltipText)
    {
        return DrawEditorToggleButton(label, isActive, ImVec2(0.0f, 0.0f), tooltipText);
    }

    bool DrawEditorToggleButton(const char* label, bool isActive, const ImVec2& buttonSize,
                                const char* tooltipText)
    {
        constexpr int styleColorCount = 3;
        if (isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.28f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        }

        const bool clicked = ImGui::Button(label, buttonSize);

        if (isActive)
            ImGui::PopStyleColor(styleColorCount);

        DrawInspectorTooltipIfHovered(tooltipText);
        return clicked;
    }

    bool DrawTableFillButton(const char* label, const char* tooltipText)
    {
        const bool clicked = ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f));
        DrawInspectorTooltipIfHovered(tooltipText);
        return clicked;
    }

    void DrawHorizontalButtonPair(const char* pairIdentifier,
                                  const std::function<void()>& drawLeftColumn,
                                  const std::function<void()>& drawRightColumn)
    {
        ImGui::PushID(pairIdentifier);
        if (ImGui::BeginTable("##HorizontalButtonPair", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            drawLeftColumn();
            ImGui::TableNextColumn();
            drawRightColumn();
            ImGui::EndTable();
        }
        ImGui::PopID();
    }

    static bool IsImageFileExtension(const std::filesystem::path& filePath)
    {
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp"
               || extension == ".tga";
    }

    bool AssignTextureFromContentBrowserPayload(const ImGuiPayload* payload, Ref<Texture2D>& texture,
                                                AssetHandle& textureHandle)
    {
        if (!payload)
            return false;

        const wchar_t* relativePathWide = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path relativePath(relativePathWide);
        std::filesystem::path textureFilePath = Project::GetAssetDirectory() / relativePath;

        if (!std::filesystem::exists(textureFilePath))
            return false;

        if (!IsImageFileExtension(textureFilePath))
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        AssetHandle importedHandle = assetManager->ImportAsset(relativePath);
        if (importedHandle == 0)
            return false;

        textureHandle = importedHandle;
        Ref<Asset> asset = assetManager->GetAsset(importedHandle);
        texture = asset ? std::static_pointer_cast<Texture2D>(asset) : nullptr;
        return texture != nullptr;
    }

    bool AssignSpriteFromContentBrowserPayload(const ImGuiPayload* payload, AssetHandle& spriteAssetHandle)
    {
        if (!payload)
            return false;

        const wchar_t* relativePathWide = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path relativePath(relativePathWide);
        std::filesystem::path textureFilePath = Project::GetAssetDirectory() / relativePath;

        if (!std::filesystem::exists(textureFilePath) || !IsImageFileExtension(textureFilePath))
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const AssetHandle textureHandle = assetManager->ImportAsset(relativePath);
        if (textureHandle == 0)
            return false;

        spriteAssetHandle = assetManager->GetDefaultSpriteHandleForTexture(textureHandle);
        return spriteAssetHandle != 0;
    }

    bool AssignAnimationAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                       AssetHandle& animationAssetHandle)
    {
        if (!payload)
            return false;

        const wchar_t* relativePathWide = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path relativePath(relativePathWide);
        std::filesystem::path animationFilePath = Project::GetAssetDirectory() / relativePath;

        if (!std::filesystem::exists(animationFilePath))
            return false;

        std::string extension = animationFilePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (extension != ".anim")
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const AssetHandle importedHandle = assetManager->ImportAsset(relativePath);
        if (importedHandle == 0)
            return false;

        animationAssetHandle = importedHandle;
        return true;
    }

    bool AssignSoundAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                   AssetHandle& soundAssetHandle)
    {
        if (!payload)
            return false;

        const wchar_t* relativePathWide = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path relativePath(relativePathWide);
        std::filesystem::path soundFilePath = Project::GetAssetDirectory() / relativePath;

        if (!std::filesystem::exists(soundFilePath))
            return false;

        std::string extension = soundFilePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (extension != ".wav" && extension != ".ogg" && extension != ".mp3")
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const AssetHandle importedHandle = assetManager->ImportAsset(relativePath);
        if (importedHandle == 0)
            return false;

        soundAssetHandle = importedHandle;
        return true;
    }

    bool AssignParticleEmitterAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                             AssetHandle& particleEmitterAssetHandle)
    {
        if (!payload)
            return false;

        const wchar_t* relativePathWide = static_cast<const wchar_t*>(payload->Data);
        std::filesystem::path relativePath(relativePathWide);
        std::filesystem::path emitterFilePath = Project::GetAssetDirectory() / relativePath;

        if (!std::filesystem::exists(emitterFilePath))
            return false;

        std::string extension = emitterFilePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        if (extension != ".particle")
            return false;

        auto assetManager = ResourceSystem::GetAssetManager();
        if (!assetManager)
            return false;

        const AssetHandle importedHandle = assetManager->ImportAsset(relativePath);
        if (importedHandle == 0)
            return false;

        particleEmitterAssetHandle = importedHandle;
        return true;
    }

    void DrawObjectReferenceField(const char* label, const char* objectDisplayName, bool hasReference,
                                  const Ref<Texture2D>& previewTexture,
                                  const std::function<void()>& onClear,
                                  const std::function<bool(const ImGuiPayload*)>& onAssignPayload,
                                  const std::function<void()>& onOpenEditor)
    {
        const bool showReset = hasReference && static_cast<bool>(onClear);
        DrawPropertyRow(
            label,
            [&]()
            {
                const float frameHeight = ImGui::GetFrameHeight();
                const float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
                const bool showPreview = previewTexture && previewTexture->GetRendererID() != 0;
                const bool showOpenEditor = static_cast<bool>(onOpenEditor);
                const float openEditorButtonWidth = showOpenEditor ? frameHeight : 0.0f;

                float availableWidth = ImGui::GetContentRegionAvail().x;
                if (showPreview)
                    availableWidth -= frameHeight + itemSpacingX;
                if (showOpenEditor)
                    availableWidth -= openEditorButtonWidth + itemSpacingX;

                const float nameFieldWidth =
                    std::max(40.0f, std::min(InspectorObjectReferenceNameMaxWidth, availableWidth));

                const char* nameText = "None";
                if (hasReference && objectDisplayName && objectDisplayName[0] != '\0')
                    nameText = objectDisplayName;

                if (showPreview)
                {
                    DrawEditorTextureImageFull(previewTexture->GetRendererID(),
                                               ImVec2(frameHeight, frameHeight));
                    ImGui::SameLine();
                }

                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                     ImGui::GetStyle().Colors[ImGuiCol_FrameBgHovered]);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                                     ImGui::GetStyle().Colors[ImGuiCol_FrameBgActive]);
                if (!hasReference)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

                ImGui::Button(nameText, ImVec2(nameFieldWidth, frameHeight));

                if (!hasReference)
                    ImGui::PopStyleColor();
                ImGui::PopStyleColor(3);

                if (ImGui::IsItemHovered() && onOpenEditor
                    && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    onOpenEditor();
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        if (onAssignPayload)
                            onAssignPayload(payload);
                    }
                    ImGui::EndDragDropTarget();
                }

                if (showOpenEditor)
                {
                    ImGui::SameLine();
                    if (ImGui::Button("##OpenEditor", ImVec2(openEditorButtonWidth, frameHeight)))
                        onOpenEditor();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    const ImVec2 buttonMin = ImGui::GetItemRectMin();
                    const ImVec2 buttonMax = ImGui::GetItemRectMax();
                    const ImVec2 center((buttonMin.x + buttonMax.x) * 0.5f,
                                       (buttonMin.y + buttonMax.y) * 0.5f);
                    const float radius = frameHeight * 0.12f;
                    const ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text);
                    drawList->AddCircleFilled(ImVec2(center.x - radius * 2.2f, center.y), radius,
                                              iconColor);
                    drawList->AddCircleFilled(center, radius, iconColor);
                    drawList->AddCircleFilled(ImVec2(center.x + radius * 2.2f, center.y), radius,
                                              iconColor);
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Open Editor");
                }
            },
            nullptr, showReset, onClear);
    }

    void DrawEditorTextureImageFull(uint64_t textureRendererId, const ImVec2& displaySize)
    {
        const auto& corners = SpriteSheetUtility::FullTextureImGuiUvCorners;
        ImGui::Image((ImTextureID)(intptr_t)textureRendererId, displaySize,
                     ImVec2(corners.TopLeft.x, corners.TopLeft.y),
                     ImVec2(corners.BottomRight.x, corners.BottomRight.y));
    }

    void DrawEditorTextureImageSubRect(uint64_t textureRendererId, const ImVec2& displaySize,
                                       const glm::ivec4& pixelRect, uint32_t textureWidth,
                                       uint32_t textureHeight)
    {
        const auto corners =
                SpriteSheetUtility::PixelRectToImGuiImageUv(pixelRect, textureWidth, textureHeight);
        ImGui::Image((ImTextureID)(intptr_t)textureRendererId, displaySize,
                     ImVec2(corners.TopLeft.x, corners.TopLeft.y),
                     ImVec2(corners.BottomRight.x, corners.BottomRight.y));
    }

} // namespace Himii
