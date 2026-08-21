#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "World/Scene/Components.h"
#include "World/Scene/SceneCamera.h"

#include <glm/gtc/constants.hpp>

namespace Himii
{
    static void DrawCameraComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<CameraComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<CameraComponent>();
        Ref<Texture2D> icon = drawContext.getComponentIcon ? drawContext.getComponentIcon("Camera") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "CameraComponent", "Camera", icon,
            [&]()
            {
                auto& camera = component.Camera;

                glm::vec4 backgroundColor = camera.GetBackgroundColor();
                const glm::vec4 previousBackgroundColor = backgroundColor;
                DrawColorControl("Background Color", backgroundColor);
                if (backgroundColor != previousBackgroundColor)
                    camera.SetBackgroundColor(backgroundColor);

                DrawCheckboxControl("Primary", component.Primary);

                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                int projectionTypeIndex = static_cast<int>(camera.GetProjectionType());
                DrawEnumComboControl(
                    "Projection", projectionTypeIndex, projectionTypeStrings, 2,
                    [&](int newIndex)
                    {
                        camera.SetProjectionType(static_cast<SceneCamera::ProjectionType>(newIndex));
                    });

                if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
                {
                    float perspectiveFieldOfView = glm::degrees(camera.GetPerspectiveVerticalFOV());
                    DrawFloatControl("Vertical FOV", perspectiveFieldOfView);
                    if (perspectiveFieldOfView != glm::degrees(camera.GetPerspectiveVerticalFOV()))
                        camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveFieldOfView));

                    float perspectiveNear = camera.GetPerspectiveNearClip();
                    DrawFloatControl("Near", perspectiveNear);
                    camera.SetPerspectiveNearClip(perspectiveNear);

                    float perspectiveFar = camera.GetPerspectiveFarClip();
                    DrawFloatControl("Far", perspectiveFar);
                    camera.SetPerspectiveFarClip(perspectiveFar);
                }

                if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
                {
                    float orthographicSize = camera.GetOrthographicSize();
                    DrawFloatControl("Size", orthographicSize);
                    camera.SetOrthographicSize(orthographicSize);

                    float orthographicNear = camera.GetOrthographicNearClip();
                    DrawFloatControl("Near", orthographicNear);
                    camera.SetOrthographicNearClip(orthographicNear);

                    float orthographicFar = camera.GetOrthographicFarClip();
                    DrawFloatControl("Far", orthographicFar);
                    camera.SetOrthographicFarClip(orthographicFar);

                    DrawCheckboxControl("Fixed Aspect Ratio", component.FixedAspectRatio);
                }

                DrawFloatControl("Exposure", component.Exposure, 0.01f, 0.0f, 0.0f, nullptr, nullptr, true, 1.0f);
            },
            [&]() { drawContext.entity.RemoveComponent<CameraComponent>(); });
    }

    struct CameraComponentInspectorRegistrar
    {
        CameraComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<CameraComponent>(
                "CameraComponent", "Camera", "Camera", 10, &DrawCameraComponentInspectorUI);
        }
    };

    static CameraComponentInspectorRegistrar s_CameraComponentInspectorRegistrar;
}
