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
    static void DrawBoxCollider2DComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<BoxCollider2DComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<BoxCollider2DComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Box Collider2D") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "BoxCollider2DComponent", "Box Collider2D", icon,
            [&]()
            {
                DrawVec2Control("Offset", component.Offset, 0.1f);
                DrawVec2Control("Size", component.Size, 0.1f, 0.0f, 0.0f, nullptr, true,
                                glm::vec2(1.0f, 1.0f));
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
            [&]() { drawContext.entity.RemoveComponent<BoxCollider2DComponent>(); });
    }

    struct BoxCollider2DComponentInspectorRegistrar
    {
        BoxCollider2DComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<BoxCollider2DComponent>(
                "BoxCollider2DComponent", "Box Collider2D", "Box Collider2D", 70,
                &DrawBoxCollider2DComponentInspectorUI);
        }
    };

    static BoxCollider2DComponentInspectorRegistrar s_BoxCollider2DComponentInspectorRegistrar;
}
