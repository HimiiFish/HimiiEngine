#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

namespace Himii
{
    static void DrawRigidbody2DComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<Rigidbody2DComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<Rigidbody2DComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Rigidbody2D") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "Rigidbody2DComponent", "Rigidbody2D", icon,
            [&]()
            {
                const char* bodyTypeStrings[] = {"Static", "Dynamic", "Kinematic"};
                int bodyTypeIndex = static_cast<int>(component.Type);
                DrawEnumComboControl(
                    "Body Type", bodyTypeIndex, bodyTypeStrings, 3,
                    [&](int newIndex)
                    {
                        component.Type = static_cast<Rigidbody2DComponent::BodyType>(newIndex);
                    });

                DrawCheckboxControl("Fixed Rotation", component.FixedRotation);
            },
            [&]() { drawContext.entity.RemoveComponent<Rigidbody2DComponent>(); });
    }

    struct Rigidbody2DComponentInspectorRegistrar
    {
        Rigidbody2DComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<Rigidbody2DComponent>(
                "Rigidbody2DComponent", "Rigidbody2D", "Rigidbody2D", 60, &DrawRigidbody2DComponentInspectorUI);
        }
    };

    static Rigidbody2DComponentInspectorRegistrar s_Rigidbody2DComponentInspectorRegistrar;
}
