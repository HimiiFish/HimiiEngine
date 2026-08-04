#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

namespace Himii
{
    static void DrawLightComponentInspectorUI(ComponentInspectorDrawContext &drawContext)
    {
        if (!drawContext.entity.HasComponent<LightComponent>())
            return;

        auto &component = drawContext.entity.GetComponent<LightComponent>();

        DrawComponentInspectorHeaderUI(
                drawContext, "LightComponent", "Light", nullptr,
                [&]()
                {
                    DrawCheckboxControl("Enabled", component.Enabled, true);

                    DrawReadOnlyTextControl(
                            "Type", "Directional",
                            "Phase 1 only supports Directional lights. Direction comes from Transform forward (-Z).");

                    DrawColorControl("Color", component.Color, glm::vec4(1.0f));
                    DrawFloatControl("Intensity", component.Intensity, 0.05f, 0.0f, 0.0f, nullptr, nullptr, true,
                                     1.0f);

                    DrawReadOnlyTextControl(
                            "Selection Rule", "First enabled Directional wins",
                            "If multiple Directional lights are enabled, only the first one in the scene affects Lit shading.");
                },
                [&]() { drawContext.entity.RemoveComponent<LightComponent>(); });
    }

    struct LightComponentInspectorRegistrar
    {
        LightComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<LightComponent>(
                    "LightComponent", "Light", "Rendering", 45, &DrawLightComponentInspectorUI);
        }
    };

    static LightComponentInspectorRegistrar s_LightComponentInspectorRegistrar;
}
