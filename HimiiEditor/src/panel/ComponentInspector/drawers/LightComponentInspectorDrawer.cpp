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

                    const char *lightTypeLabels[] = {"Directional", "Point"};
                    int lightTypeIndex = component.Type == LightType::Point ? 1 : 0;
                    DrawEnumComboControl(
                            "Type", lightTypeIndex, lightTypeLabels, 2,
                            [&](int newIndex)
                            {
                                component.Type =
                                        newIndex == 1 ? LightType::Point : LightType::Directional;
                            },
                            true, 0);

                    DrawColorControl("Color", component.Color, glm::vec4(1.0f));
                    DrawFloatControl("Intensity", component.Intensity, 0.05f, 0.0f, 0.0f, nullptr, nullptr, true,
                                     1.0f);

                    if (component.Type == LightType::Point)
                    {
                        DrawFloatControl("Range", component.Range, 0.1f, 0.01f, 0.0f, nullptr, nullptr, true,
                                         10.0f);
                        DrawReadOnlyTextControl(
                                "Selection Rule", "Up to 8 enabled Point lights",
                                "Lit accumulates up to eight enabled Point lights in entity order. Range is in world units and ignores Transform scale.");
                    }
                    else
                    {
                        DrawCheckboxControl("Cast Shadows", component.CastShadows, true);
                        DrawFloatControl("Shadow Distance", component.ShadowDistance, 0.5f, 0.1f, 0.0f, nullptr, nullptr,
                                         true, 80.0f);

                        const char *resolutionLabels[] = {"1024", "2048", "4096"};
                        int resolutionIndex = static_cast<int>(component.ShadowMapResolution);
                        if (resolutionIndex < 0 || resolutionIndex > 2)
                        {
                            resolutionIndex = static_cast<int>(Himii::ShadowMapResolution::Pixels2048);
                            component.ShadowMapResolution = Himii::ShadowMapResolution::Pixels2048;
                        }
                        DrawEnumComboControl(
                                "Shadow Map Resolution", resolutionIndex, resolutionLabels, 3,
                                [&](int newIndex)
                                {
                                    component.ShadowMapResolution =
                                            static_cast<Himii::ShadowMapResolution>(newIndex);
                                },
                                true, static_cast<int>(Himii::ShadowMapResolution::Pixels2048));

                        DrawReadOnlyTextControl(
                                "Selection Rule", "First enabled Directional wins",
                                "Lit uses the first enabled Directional light. Shadows run when that light also has Cast Shadows enabled, covering the view out to Shadow Distance. Shadow Map Resolution is the atlas edge length. Direction comes from Transform forward (-Z).");
                    }
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
