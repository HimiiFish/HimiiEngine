#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Himii/Scene/Components.h"

namespace Himii
{
    static void DrawUIButtonComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<UIButtonComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<UIButtonComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Button") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "UIButtonComponent", "Button", icon,
            [&]()
            {
                DrawInspectorSectionHeader("Interaction");
                DrawCheckboxControl("Interactable", component.Interactable);

                DrawInspectorSectionHeader("Style");
                DrawColorControl("Normal", component.Colors.NormalColor);
                DrawColorControl("Highlighted", component.Colors.HighlightedColor);
                DrawColorControl("Pressed", component.Colors.PressedColor);
                DrawColorControl("Disabled", component.Colors.DisabledColor);

                if (!drawContext.entity.HasComponent<UIImageComponent>())
                {
                    DrawReadOnlyTextControl(
                        "Image", "None (invisible hit area)",
                        "未挂 Image 时，Button 仅作为透明点击区域，类似 UE 无刷按钮。");
                }
            },
            [&]() { drawContext.entity.RemoveComponent<UIButtonComponent>(); });
    }

    struct UIButtonComponentInspectorRegistrar
    {
        UIButtonComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<UIButtonComponent>(
                "UIButtonComponent", "Button", "Button", 220, &DrawUIButtonComponentInspectorUI);
        }
    };

    static UIButtonComponentInspectorRegistrar s_UIButtonComponentInspectorRegistrar;
}
