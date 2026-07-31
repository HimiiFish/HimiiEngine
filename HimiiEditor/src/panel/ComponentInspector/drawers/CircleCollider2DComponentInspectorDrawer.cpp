#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Project/Project.h"
#include "World/Scene/Components.h"
#include "Project/Physics2DLayerSettings.h"

#include <algorithm>

namespace Himii
{
    static void DrawCircleCollider2DComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<CircleCollider2DComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<CircleCollider2DComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Circle Collider2D") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "CircleCollider2DComponent", "Circle Collider2D", icon,
            [&]()
            {
                DrawVec2Control("Offset", component.Offset, 0.1f);
                DrawFloatControl("Radius", component.Radius, 0.1f, 0.0f, 0.0f, nullptr, nullptr, true,
                                 0.5f);
                DrawFloatControl("Density", component.Density, 0.1f, 0.0f, 0.0f, nullptr, nullptr, true,
                                 1.0f);
                DrawFloatControl("Friction", component.Friction, 0.01f, 0.0f, 1.0f, nullptr, nullptr,
                                 true, 0.5f);
                DrawFloatControl("Restitution", component.Restitution, 0.01f, 0.0f, 1.0f, nullptr,
                                 nullptr, true, 0.0f);
                DrawFloatControl("Restitution Threshold", component.RestitutionThreshold, 0.1f, 0.0f,
                                 0.0f, nullptr, nullptr, true, 0.5f);
                DrawCheckboxControl("Is Trigger", component.IsTrigger, false);

                component.Layer = std::clamp(component.Layer, 0, Physics2DLayerCount - 1);

                int physicsLayerIndex = component.Layer;
                Physics2DLayerSettings physicsLayerSettings;
                if (Project::GetActive())
                    physicsLayerSettings = Project::GetConfig().Physics2DLayers;

                std::vector<const char*> physicsLayerLabels;
                physicsLayerLabels.reserve(Physics2DLayerCount);
                for (int layerIndex = 0; layerIndex < Physics2DLayerCount; ++layerIndex)
                    physicsLayerLabels.push_back(physicsLayerSettings.LayerNames[layerIndex].c_str());

                DrawEnumComboControl(
                    "Layer", physicsLayerIndex, physicsLayerLabels.data(), Physics2DLayerCount,
                    [&](int newIndex) { component.Layer = newIndex; });
            },
            [&]() { drawContext.entity.RemoveComponent<CircleCollider2DComponent>(); });
    }

    struct CircleCollider2DComponentInspectorRegistrar
    {
        CircleCollider2DComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<CircleCollider2DComponent>(
                "CircleCollider2DComponent", "Circle Collider2D", "Circle Collider2D", 80,
                &DrawCircleCollider2DComponentInspectorUI);
        }
    };

    static CircleCollider2DComponentInspectorRegistrar s_CircleCollider2DComponentInspectorRegistrar;
}
