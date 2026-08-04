#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

namespace Himii
{
    static void DrawEnvironmentComponentInspectorUI(ComponentInspectorDrawContext &drawContext)
    {
        if (!drawContext.entity.HasComponent<EnvironmentComponent>())
            return;

        auto &component = drawContext.entity.GetComponent<EnvironmentComponent>();

        DrawComponentInspectorHeaderUI(
                drawContext, "EnvironmentComponent", "Environment", nullptr,
                [&]()
                {
                    DrawCheckboxControl("Enabled", component.Enabled, true);
                    DrawColorControl("Ambient Color", component.AmbientColor, glm::vec4(1.0f));
                    DrawFloatControl("Ambient Intensity", component.AmbientIntensity, 0.01f, 0.0f, 0.0f, nullptr,
                                     nullptr, true, 0.15f);

                    DrawReadOnlyTextControl(
                            "Note", "Requires Directional Light",
                            "Ambient only contributes when an enabled Directional Light exists. First enabled Environment wins.");
                },
                [&]() { drawContext.entity.RemoveComponent<EnvironmentComponent>(); });
    }

    struct EnvironmentComponentInspectorRegistrar
    {
        EnvironmentComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<EnvironmentComponent>(
                    "EnvironmentComponent", "Environment", "Rendering", 46,
                    &DrawEnvironmentComponentInspectorUI);
        }
    };

    static EnvironmentComponentInspectorRegistrar s_EnvironmentComponentInspectorRegistrar;
}
