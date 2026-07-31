#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

#include <glm/glm.hpp>

namespace Himii
{
    static void DrawCanvasComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<CanvasComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<CanvasComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Canvas") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "CanvasComponent", "Canvas", icon,
            [&]()
            {
                DrawInspectorSectionHeader("Render");
                DrawReadOnlyTextControl(
                    "Render Mode", "Screen Space Overlay",
                    "当前仅支持屏幕空间叠加；世界空间模式尚未开放。");

                DrawInspectorSectionHeader("Scaler");
                {
                    const char* scaleModeLabels[] = {
                        "Constant Pixel Size", "Scale With Screen Size"};
                    int scaleModeIndex = static_cast<int>(component.ScaleMode);
                    DrawEnumComboControl(
                        "Scale Mode", scaleModeIndex, scaleModeLabels, 2,
                        [&](int newIndex)
                        {
                            component.ScaleMode = static_cast<CanvasScaleMode>(newIndex);
                        });
                }

                DrawVec2Control("Reference Resolution", component.ReferenceResolution, 1.0f, 1.0f,
                                16384.0f, nullptr, true, glm::vec2(1920.0f, 1080.0f));

                if (component.ScaleMode == CanvasScaleMode::ScaleWithScreenSize)
                {
                    DrawFloatControl("Match Width Or Height", component.MatchWidthOrHeight, 0.01f,
                                     0.0f, 1.0f, nullptr, nullptr, true, 0.5f);
                    component.MatchWidthOrHeight =
                        glm::clamp(component.MatchWidthOrHeight, 0.0f, 1.0f);
                }

                if (drawContext.scene)
                    drawContext.scene->SyncCanvasReferenceResolutionToTransform(drawContext.entity);
            },
            [&]() { drawContext.entity.RemoveComponent<CanvasComponent>(); });
    }

    struct CanvasComponentInspectorRegistrar
    {
        CanvasComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<CanvasComponent>(
                "CanvasComponent", "Canvas", "Canvas", 190, &DrawCanvasComponentInspectorUI);
        }
    };

    static CanvasComponentInspectorRegistrar s_CanvasComponentInspectorRegistrar;
}
