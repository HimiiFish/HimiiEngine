#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Himii/Renderer/Font.h"
#include "Himii/Scene/Components.h"

#include <imgui.h>

namespace Himii
{
    static void DrawUITextComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<UITextComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<UITextComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Text") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "UITextComponent", "Text", icon,
            [&]()
            {
                DrawInspectorSectionHeader("Content");
                DrawMultilineTextControl("Text", component.TextString, 3);

                DrawInspectorSectionHeader("Appearance");
                DrawColorControl("Color", component.Color);
                DrawFloatControl("Font Size", component.FontSize, 1.0f, 0.0f, 0.0f, nullptr, nullptr,
                                 true, 48.0f);
                if (component.FontSize < 1.0f)
                    component.FontSize = 1.0f;
                DrawFloatControl("Kerning", component.Kerning, 0.01f);
                DrawFloatControl("Line Spacing", component.LineSpacing, 0.01f);

                DrawReadOnlyTextControl(
                    "Font", component.FontAsset ? "Default Font Assigned" : "None (Default)",
                    "当前使用引擎默认字体资源；自定义字体选择后续开放。");
                if (!component.FontAsset)
                {
                    DrawActionButtonRow("Font", [&]()
                    {
                        if (ImGui::Button("Attach Default Font", ImVec2(-1.0f, 0.0f)))
                            component.FontAsset = Font::GetDefault();
                    });
                }

                DrawInspectorSectionHeader("Alignment");
                {
                    static const char* horizontalAlignmentLabels[] = {"Left", "Center", "Right"};
                    int horizontalAlignmentIndex = static_cast<int>(component.HorizontalAlignment);
                    DrawEnumComboControl(
                        "Horizontal", horizontalAlignmentIndex, horizontalAlignmentLabels, 3,
                        [&](int selectedIndex)
                        {
                            component.HorizontalAlignment =
                                static_cast<TextHorizontalAlignment>(selectedIndex);
                        });
                }
                {
                    static const char* verticalAlignmentLabels[] = {"Top", "Middle", "Bottom"};
                    int verticalAlignmentIndex = static_cast<int>(component.VerticalAlignment);
                    DrawEnumComboControl(
                        "Vertical", verticalAlignmentIndex, verticalAlignmentLabels, 3,
                        [&](int selectedIndex)
                        {
                            component.VerticalAlignment =
                                static_cast<TextVerticalAlignment>(selectedIndex);
                        });
                }
            },
            [&]() { drawContext.entity.RemoveComponent<UITextComponent>(); });
    }

    struct UITextComponentInspectorRegistrar
    {
        UITextComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<UITextComponent>(
                "UITextComponent", "Text", "Text", 220, &DrawUITextComponentInspectorUI);
        }
    };

    static UITextComponentInspectorRegistrar s_UITextComponentInspectorRegistrar;
}
