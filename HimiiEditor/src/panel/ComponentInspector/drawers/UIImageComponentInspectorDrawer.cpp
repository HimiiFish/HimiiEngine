#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "panel/ComponentInspector/ComponentInspectorUtils.h"
#include "InspectorControls.h"

#include "Himii/Scene/Components.h"

namespace Himii
{
    static void DrawUIImageComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<UIImageComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<UIImageComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Image") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "UIImageComponent", "Image", icon,
            [&]()
            {
                DrawInspectorSectionHeader("Appearance");
                DrawTextureAssignControl("Texture", component.Texture, component.TextureHandle);
                DrawColorControl("Color", component.Color);
            },
            [&]() { drawContext.entity.RemoveComponent<UIImageComponent>(); });
    }

    struct UIImageComponentInspectorRegistrar
    {
        UIImageComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<UIImageComponent>(
                "UIImageComponent", "Image", "Image", 210, &DrawUIImageComponentInspectorUI);
        }
    };

    static UIImageComponentInspectorRegistrar s_UIImageComponentInspectorRegistrar;
}
