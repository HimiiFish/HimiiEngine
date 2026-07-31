#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

namespace Himii
{
    static void DrawCircleRendererComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<CircleRendererComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<CircleRendererComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Circle Renderer") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "CircleRendererComponent", "Circle Renderer", icon,
            [&]()
            {
                DrawColorControl("Color", component.Color);
                DrawFloatControl("Thickness", component.Thickness, 0.025f, 0.0f, 1.0f, nullptr, nullptr,
                                 true, 1.0f);
                DrawFloatControl("Fade", component.Fade, 0.0003f, 0.0f, 1.0f, nullptr, nullptr, true,
                                 0.005f);
            },
            [&]() { drawContext.entity.RemoveComponent<CircleRendererComponent>(); });
    }

    struct CircleRendererComponentInspectorRegistrar
    {
        CircleRendererComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<CircleRendererComponent>(
                "CircleRendererComponent", "Circle Renderer", "Circle Renderer", 50,
                &DrawCircleRendererComponentInspectorUI);
        }
    };

    static CircleRendererComponentInspectorRegistrar s_CircleRendererComponentInspectorRegistrar;
}
