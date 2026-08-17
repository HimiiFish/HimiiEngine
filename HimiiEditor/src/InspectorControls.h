#pragma once

#include "Resource/Asset.h"
#include "EngineCore/Core/Core.h"
#include "Module/Render/RenderCore/Texture.h"
#include "Module/Tilemap/TileSet.h"

#include <glm/glm.hpp>
#include <cstdint>

#include <functional>
#include <string>

struct ImGuiPayload;
struct ImVec2;

namespace Himii
{

    constexpr float InspectorLabelColumnWidth = 140.0f;
    constexpr float InspectorResetColumnWidth = 22.0f;
    constexpr float InspectorObjectReferenceNameMaxWidth = 200.0f;

    /** Properties 面板内收紧行距（成对调用）。 */
    void BeginInspectorPropertiesStyle();
    void EndInspectorPropertiesStyle();

    /**
     * 属性行：标签 | 值 | Reset（可选）。
     * showResetButton 为 true 且提供 onReset 时，仅在调用方判定「已偏离默认」时传入 true。
     */
    void DrawPropertyRow(const char* label, const std::function<void()>& drawValueColumn,
                         const char* tooltipText = nullptr, bool showResetButton = false,
                         const std::function<void()>& onReset = nullptr);

    /** 对上一项控件在鼠标悬浮时显示提示（通常紧接在 DrawPropertyRow 等之后调用）。 */
    void DrawInspectorTooltipIfHovered(const char* tooltipText);

    void DrawFloatControl(const std::string& label, float& value, float speed = 0.1f,
                          float minimum = 0.0f, float maximum = 0.0f,
                          const std::function<void()>& onEditBegin = nullptr,
                          const std::function<void()>& onEditEnd = nullptr,
                          bool enableRowReset = true, float resetValue = 0.0f);

    void DrawIntControl(const char* label, int& value, float speed = 1.0f, int minimum = 0,
                        int maximum = 0, bool enableRowReset = true, int resetValue = 0);

    void DrawColorControl(const std::string& label, glm::vec4& value,
                          const glm::vec4& resetValue = glm::vec4(1.0f));

    void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f,
                         const std::function<void()>& onEditBegin = nullptr,
                         const std::function<void()>& onEditEnd = nullptr,
                         bool enableRowReset = true);

    void DrawVec2AxisControl(const std::string& label, glm::vec2& values, float resetValue = 0.0f,
                             const std::function<void()>& onEditBegin = nullptr,
                             const std::function<void()>& onEditEnd = nullptr,
                             bool enableRowReset = true);

    void DrawStdStringControl(const char* label, std::string& value,
                              const std::function<void()>& onEdited = nullptr,
                              bool enableRowReset = true, const std::string& resetValue = "");

    void DrawCheckboxControl(const std::string& label, bool& value, bool resetValue = false,
                             bool enableRowReset = true);

    void DrawEnumComboControl(const char* label, int& currentIndex, const char* const* labels,
                              int labelCount, const std::function<void(int newIndex)>& onSelectionChanged,
                              bool enableRowReset = false, int resetIndex = 0);

    void DrawRuleTileNeighborMatrixControl(
            const char* label,
            RuleTileNeighborCondition neighborConditions[RuleTileNeighborCount],
            const std::function<void()>& onEdited = nullptr);

    void DrawUInt32Control(const char* label, uint32_t& value, float speed = 1.0f,
                           bool enableRowReset = true, uint32_t resetValue = 0);

    void DrawIVec2Control(const char* label, glm::ivec2& value, float speed = 1.0f,
                          int minimum = 0, int maximum = 0, bool enableRowReset = true,
                          glm::ivec2 resetValue = glm::ivec2(0));

    void DrawIVec4Control(const char* label, glm::ivec4& value, float speed = 1.0f,
                          int minimum = 0, int maximum = 8192,
                          const std::function<void()>& onEdited = nullptr,
                          bool enableRowReset = true, glm::ivec4 resetValue = glm::ivec4(0));

    void DrawInputTextControl(const char* label, char* buffer, int bufferSize,
                              const std::function<void()>& onEdited = nullptr);

    /** 多行文本（如 UI Text Content）。行高约 lineCount 行。 */
    void DrawMultilineTextControl(const char* label, std::string& value, int lineCount = 3,
                                  const std::function<void()>& onEdited = nullptr);

    void DrawVec2Control(const char* label, glm::vec2& value, float speed = 0.01f,
                         float minimum = 0.0f, float maximum = 0.0f,
                         const std::function<void()>& onEdited = nullptr,
                         bool enableRowReset = true, glm::vec2 resetValue = glm::vec2(0.0f));

    void DrawReadOnlyTextControl(const char* label, const char* text,
                                 const char* tooltipText = nullptr);

    void DrawInspectorSectionHeader(const char* title, const char* tooltipText = nullptr);

    void DrawActionButtonRow(const char* label, const std::function<void()>& drawButtons);

    /** 与编辑器主工具栏一致：未选中透明，选中时深色底。返回是否在本帧被点击。 */
    bool DrawEditorToggleButton(const char* label, bool isActive, const char* tooltipText = nullptr);
    bool DrawEditorToggleButton(const char* label, bool isActive, const ImVec2& buttonSize,
                                const char* tooltipText = nullptr);

    /** 两列等宽按钮行（用于 Save、批量操作等）。 */
    void DrawHorizontalButtonPair(const char* pairIdentifier,
                                  const std::function<void()>& drawLeftColumn,
                                  const std::function<void()>& drawRightColumn);

    /**
     * 在 Table 单元格内铺满宽度的按钮。勿使用 ImVec2(-1,0)，否则列宽会每帧被错误回传并逐渐变窄。
     */
    bool DrawTableFillButton(const char* label, const char* tooltipText = nullptr);

    bool AssignTextureFromContentBrowserPayload(const ImGuiPayload* payload, Ref<Texture2D>& texture,
                                                AssetHandle& textureHandle);

    bool AssignSpriteFromContentBrowserPayload(const ImGuiPayload* payload, AssetHandle& spriteAssetHandle);

    bool AssignAnimationAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                       AssetHandle& animationAssetHandle);

    bool AssignSoundAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                   AssetHandle& soundAssetHandle);

    bool AssignParticleEmitterAssetFromContentBrowserPayload(const ImGuiPayload* payload,
                                                             AssetHandle& particleEmitterAssetHandle);

    bool AssignMeshAssetFromContentBrowserPayload(const ImGuiPayload *payload, AssetHandle &meshAssetHandle);
    bool AssignMaterialAssetFromContentBrowserPayload(const ImGuiPayload *payload,
                                                      AssetHandle &materialAssetHandle);

    /**
     * 紧凑引用字段：缩略图 + 限宽名称 + 可选「打开 Editor」。
     * 行级 Reset（清空）在有引用时显示；onOpenEditor 同时用于双击与名称旁按钮。
     */
    void DrawObjectReferenceField(const char* label, const char* objectDisplayName, bool hasReference,
                                  const Ref<Texture2D>& previewTexture,
                                  const std::function<void()>& onClear,
                                  const std::function<bool(const ImGuiPayload*)>& onAssignPayload,
                                  const std::function<void()>& onOpenEditor = nullptr);

    /** 编辑器内显示整张纹理（与 SpriteSheetUtility 约定一致，避免上下颠倒）。 */
    void DrawEditorTextureImageFull(uint64_t textureRendererId, const ImVec2& displaySize);

    /** 编辑器内显示 PixelRect 子区域。 */
    void DrawEditorTextureImageSubRect(uint64_t textureRendererId, const ImVec2& displaySize,
                                       const glm::ivec4& pixelRect, uint32_t textureWidth,
                                       uint32_t textureHeight);

} // namespace Himii
