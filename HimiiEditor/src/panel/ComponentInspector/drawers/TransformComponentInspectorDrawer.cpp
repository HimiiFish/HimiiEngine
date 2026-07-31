#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"

#include "World/Scene/Components.h"
#include "InspectorControls.h"
#include "commands/EditorCommandHistory.h"
#include "commands/EditorCommands.h"

#include <glm/gtc/constants.hpp>

namespace Himii
{
    static void DrawTransformComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<TransformComponent>())
            return;

        TransformComponent& component = drawContext.entity.GetComponent<TransformComponent>();

        const Ref<Texture2D> componentIcon = drawContext.getComponentIcon
                                                   ? drawContext.getComponentIcon("Transform")
                                                   : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext,
            "TransformComponent",
            "Transform",
            componentIcon,
            [&]()
            {
                TransformComponent transformBeforeEdit = component;
                bool transformEditCaptured = false;

                auto beginTransformEdit = [&]()
                {
                    if (!transformEditCaptured)
                    {
                        transformBeforeEdit = component;
                        transformEditCaptured = true;
                    }
                };

                auto notifyTransformPreview = [&]()
                {
                    if (drawContext.scene)
                        drawContext.scene->MarkEntityTransformDirty(drawContext.entity);
                };

                auto endPositionScaleEdit = [&]()
                {
                    notifyTransformPreview();

                    if (!transformEditCaptured || drawContext.commandHistory == nullptr)
                    {
                        transformEditCaptured = false;
                        return;
                    }

                    TransformComponent transformAfterEdit = component;
                    if (transformBeforeEdit.Position != transformAfterEdit.Position
                        || transformBeforeEdit.Rotation != transformAfterEdit.Rotation
                        || transformBeforeEdit.Scale != transformAfterEdit.Scale)
                    {
                        drawContext.commandHistory->Execute(CreateScope<ModifyTransformCommand>(
                            drawContext.scene,
                            drawContext.entity.GetUUID(),
                            transformBeforeEdit,
                            transformAfterEdit));
                    }

                    transformEditCaptured = false;
                };

                glm::vec3 rotationDegrees = glm::degrees(component.Rotation);
                auto endRotationEdit = [&]()
                {
                    component.Rotation = glm::radians(rotationDegrees);
                    notifyTransformPreview();

                    if (!transformEditCaptured || drawContext.commandHistory == nullptr)
                    {
                        transformEditCaptured = false;
                        return;
                    }

                    TransformComponent transformAfterEdit = component;
                    if (transformBeforeEdit.Position != transformAfterEdit.Position
                        || transformBeforeEdit.Rotation != transformAfterEdit.Rotation
                        || transformBeforeEdit.Scale != transformAfterEdit.Scale)
                    {
                        drawContext.commandHistory->Execute(CreateScope<ModifyTransformCommand>(
                            drawContext.scene,
                            drawContext.entity.GetUUID(),
                            transformBeforeEdit,
                            transformAfterEdit));
                    }

                    transformEditCaptured = false;
                };

                DrawVec3Control("Position",
                                component.Position,
                                0.0f,
                                beginTransformEdit,
                                endPositionScaleEdit);

                DrawVec3Control("Rotation",
                                rotationDegrees,
                                0.0f,
                                beginTransformEdit,
                                endRotationEdit);

                component.Rotation = glm::radians(rotationDegrees);

                DrawVec3Control("Scale",
                                component.Scale,
                                1.0f,
                                beginTransformEdit,
                                endPositionScaleEdit);

                if (transformEditCaptured)
                    notifyTransformPreview();
            },
            [&]()
            {
                drawContext.entity.RemoveComponent<TransformComponent>();
            });
    }

    struct TransformComponentInspectorDrawerRegistrar
    {
        TransformComponentInspectorDrawerRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<TransformComponent>(
                "TransformComponent",
                "Transform",
                "Transform",
                0,
                &DrawTransformComponentInspectorUI);
        }
    };

    static TransformComponentInspectorDrawerRegistrar s_TransformComponentInspectorDrawerRegistrar;
} // namespace Himii

