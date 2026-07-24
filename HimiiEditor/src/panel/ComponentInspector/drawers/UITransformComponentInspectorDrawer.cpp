#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"
#include "commands/EditorCommandHistory.h"
#include "commands/EditorCommands.h"

#include "Himii/Scene/Components.h"

#include <cstdio>
#include <glm/gtc/constants.hpp>

namespace Himii
{
    static void DrawRectTransformComponentInspectorUserInterface(
            ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<RectTransformComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<RectTransformComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Rect Transform") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "RectTransformComponent", "Rect Transform", icon,
            [&]()
            {
                if (drawContext.entity.HasComponent<CanvasComponent>())
                {
                    if (drawContext.scene)
                        drawContext.scene->SyncCanvasReferenceResolutionToTransform(drawContext.entity);

                    DrawInspectorSectionHeader("Layout");
                    DrawReadOnlyTextControl(
                        "Driver", "Game Target Size",
                        "Canvas 根节点尺寸由 Game 目标分辨率驱动，类似 UE 根控件填满视口。");

                    char resolvedSizeBuffer[64];
                    std::snprintf(resolvedSizeBuffer, sizeof(resolvedSizeBuffer), "%.0f x %.0f",
                                  component.ResolvedSize.x, component.ResolvedSize.y);
                    DrawReadOnlyTextControl("Resolved Size", resolvedSizeBuffer,
                                           "当前同步后的根矩形像素尺寸。");
                    return;
                }

                RectTransformComponent transformBeforeEdit = component;
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

                auto endTransformEdit = [&]()
                {
                    notifyTransformPreview();

                    if (!transformEditCaptured || drawContext.commandHistory == nullptr)
                    {
                        transformEditCaptured = false;
                        return;
                    }

                    RectTransformComponent transformAfterEdit = component;
                    if (transformBeforeEdit.AnchorMinimum != transformAfterEdit.AnchorMinimum
                        || transformBeforeEdit.AnchorMaximum != transformAfterEdit.AnchorMaximum
                        || transformBeforeEdit.Pivot != transformAfterEdit.Pivot
                        || transformBeforeEdit.AnchoredPosition != transformAfterEdit.AnchoredPosition
                        || transformBeforeEdit.RotationRadians
                                != transformAfterEdit.RotationRadians
                        || transformBeforeEdit.SizeDelta != transformAfterEdit.SizeDelta)
                    {
                        drawContext.commandHistory->Execute(std::make_unique<ModifyRectTransformCommand>(
                            drawContext.scene,
                            drawContext.entity.GetUUID(),
                            transformBeforeEdit,
                            transformAfterEdit));
                    }
                    transformEditCaptured = false;
                };

                DrawInspectorSectionHeader("Anchors");
                DrawVec2AxisControl(
                        "Anchor Minimum", component.AnchorMinimum, 0.5f,
                        beginTransformEdit, endTransformEdit);
                component.AnchorMinimum = glm::clamp(
                        component.AnchorMinimum, glm::vec2(0.0f), glm::vec2(1.0f));

                DrawVec2AxisControl(
                        "Anchor Maximum", component.AnchorMaximum, 0.5f,
                        beginTransformEdit, endTransformEdit);
                component.AnchorMaximum = glm::clamp(
                        component.AnchorMaximum, component.AnchorMinimum, glm::vec2(1.0f));

                DrawInspectorSectionHeader("Transform");
                DrawVec2AxisControl(
                        "Pivot", component.Pivot, 0.5f,
                        beginTransformEdit, endTransformEdit);
                component.Pivot = glm::clamp(
                        component.Pivot, glm::vec2(0.0f), glm::vec2(1.0f));

                DrawVec2AxisControl(
                        "Anchored Position", component.AnchoredPosition, 0.0f,
                        beginTransformEdit, endTransformEdit);

                float rotationDegrees = glm::degrees(component.RotationRadians);
                DrawFloatControl(
                    "Rotation", rotationDegrees, 0.1f, 0.0f, 0.0f,
                    beginTransformEdit,
                    [&]()
                    {
                        component.RotationRadians = glm::radians(rotationDegrees);
                        endTransformEdit();
                    });
                component.RotationRadians = glm::radians(rotationDegrees);

                DrawVec2AxisControl(
                        "Size Delta", component.SizeDelta, 100.0f,
                        beginTransformEdit, endTransformEdit);

                if (transformEditCaptured)
                    notifyTransformPreview();
            },
            [&]() { drawContext.entity.RemoveComponent<RectTransformComponent>(); });
    }

    struct RectTransformComponentInspectorRegistrar
    {
        RectTransformComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<RectTransformComponent>(
                "RectTransformComponent", "Rect Transform", "Rect Transform", 200,
                &DrawRectTransformComponentInspectorUserInterface);
        }
    };

    static RectTransformComponentInspectorRegistrar
            s_RectTransformComponentInspectorRegistrar;
}
