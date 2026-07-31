#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"

namespace Himii
{
    static void DrawMeshComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<MeshComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<MeshComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Mesh Renderer") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "MeshComponent", "Mesh Renderer", icon,
            [&]()
            {
                const char* meshTypeStrings[] = {"Cube", "Plane", "Sphere", "Capsule"};
                int meshTypeIndex = static_cast<int>(component.Type);
                DrawEnumComboControl(
                    "Mesh Type", meshTypeIndex, meshTypeStrings, 4,
                    [&](int newIndex)
                    {
                        component.Type = static_cast<MeshComponent::MeshType>(newIndex);
                    });

                DrawColorControl("Color", component.Color);
            },
            [&]() { drawContext.entity.RemoveComponent<MeshComponent>(); });
    }

    struct MeshComponentInspectorRegistrar
    {
        MeshComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<MeshComponent>(
                "MeshComponent", "Mesh Renderer", "Mesh Renderer", 40, &DrawMeshComponentInspectorUI);
        }
    };

    static MeshComponentInspectorRegistrar s_MeshComponentInspectorRegistrar;
}
