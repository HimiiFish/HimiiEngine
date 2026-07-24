#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Himii/Asset/AssetManager.h"
#include "Himii/Project/Project.h"
#include "Himii/Scene/Components.h"
#include "Himii/Scene/SpriteAnimation.h"
#include "Himii/Scene/SpriteAnimationUtility.h"

#include <imgui.h>

namespace Himii
{
    static void DrawSpriteAnimationComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<SpriteAnimationComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<SpriteAnimationComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Sprite Animation") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "SpriteAnimationComponent", "Sprite Animation", icon,
            [&]()
            {
                if (!drawContext.entity.HasComponent<SpriteRendererComponent>())
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                                       "需要 Sprite Renderer 组件才能显示动画。");

                auto assetManager = Project::GetAssetManager();

                std::string animationDisplayName = "None";
                if (component.AnimationHandle != 0 && assetManager
                    && assetManager->IsAssetHandleValid(component.AnimationHandle))
                {
                    const auto& metadata =
                        assetManager->GetAssetRegistry().at(component.AnimationHandle);
                    animationDisplayName = metadata.FilePath.filename().string();
                }

                DrawObjectReferenceField(
                    "Animation", animationDisplayName.c_str(), component.AnimationHandle != 0, nullptr,
                    [&]()
                    {
                        component.AnimationHandle = 0;
                    },
                    [&](const ImGuiPayload* payload)
                    {
                        const bool assigned = AssignAnimationAssetFromContentBrowserPayload(
                            payload, component.AnimationHandle);
                        if (assigned && assetManager
                            && assetManager->IsAssetHandleValid(component.AnimationHandle))
                        {
                            Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                                assetManager->GetAsset(component.AnimationHandle));
                            if (animation)
                            {
                                if (component.CurrentAnimationName.empty()
                                    || !animation->HasAnimation(component.CurrentAnimationName))
                                {
                                    if (const SpriteAnimationClip* primaryClip = animation->GetPrimaryClip())
                                        component.CurrentAnimationName = primaryClip->Name;
                                }
                                if (component.FrameRate <= 0.0f)
                                    component.FrameRate =
                                        animation->GetClipFrameRate(component.CurrentAnimationName);
                            }
                        }
                        return assigned;
                    },
                    [&]()
                    {
                        if (component.AnimationHandle == 0 || !assetManager
                            || !assetManager->IsAssetHandleValid(component.AnimationHandle)
                            || !drawContext.requestAnimationEditor)
                            return;

                        drawContext.requestAnimationEditor(
                            Project::GetAssetFileSystemPath(
                                assetManager->GetAssetRegistry().at(component.AnimationHandle).FilePath));
                    });

                const std::vector<std::string> animationNames = [&]() -> std::vector<std::string>
                {
                    if (!assetManager || component.AnimationHandle == 0
                        || !assetManager->IsAssetHandleValid(component.AnimationHandle))
                        return {};
                    Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                        assetManager->GetAsset(component.AnimationHandle));
                    return animation ? animation->GetAnimationNames() : std::vector<std::string>{};
                }();

                if (!animationNames.empty())
                {
                    int selectedAnimationIndex = 0;
                    for (size_t nameIndex = 0; nameIndex < animationNames.size(); ++nameIndex)
                    {
                        if (animationNames[nameIndex] == component.CurrentAnimationName)
                        {
                            selectedAnimationIndex = static_cast<int>(nameIndex);
                            break;
                        }
                    }

                    std::vector<const char*> animationNameLabels;
                    animationNameLabels.reserve(animationNames.size());
                    for (const std::string& animationName : animationNames)
                        animationNameLabels.push_back(animationName.c_str());

                    DrawEnumComboControl(
                        "Current Animation", selectedAnimationIndex, animationNameLabels.data(),
                        static_cast<int>(animationNameLabels.size()),
                        [&](int newIndex)
                        {
                            if (newIndex >= 0 && newIndex < static_cast<int>(animationNames.size()))
                            {
                                component.CurrentAnimationName = animationNames[static_cast<size_t>(newIndex)];
                                ResetSpriteAnimationPlayback(component);
                            }
                        });
                }
                else
                {
                    DrawReadOnlyTextControl("Current Animation", component.CurrentAnimationName.c_str());
                }

                DrawFloatControl("Frame Rate", component.FrameRate, 0.1f, 0.0f, 120.0f, nullptr, nullptr,
                                 true, 10.0f);
                DrawCheckboxControl("Playing", component.Playing, true);
                DrawCheckboxControl("Preview In Scene", component.PreviewInScene, false);

                const int frameCount = [&]() -> int
                {
                    if (!assetManager || component.AnimationHandle == 0
                        || !assetManager->IsAssetHandleValid(component.AnimationHandle))
                        return 0;
                    Ref<SpriteAnimation> animation = std::static_pointer_cast<SpriteAnimation>(
                        assetManager->GetAsset(component.AnimationHandle));
                    return animation
                        ? static_cast<int>(animation->GetFrameCount(component.CurrentAnimationName))
                        : 0;
                }();

                if (frameCount > 0)
                {
                    int currentFrame = component.CurrentFrame;
                    DrawPropertyRow(
                        "Current Frame",
                        [&]()
                        {
                            ImGui::PushItemWidth(-1.0f);
                            ImGui::SliderInt("##Value", &currentFrame, 0, frameCount - 1);
                            ImGui::PopItemWidth();
                        },
                        "编辑模式下 scrub 预览帧（需勾选 Preview In Scene）。");
                    component.CurrentFrame = currentFrame;
                }
                else
                {
                    DrawReadOnlyTextControl("Current Frame", "0");
                }

                DrawHorizontalButtonPair(
                    "AnimationPlayback",
                    [&]()
                    {
                        if (DrawTableFillButton("Reset Playback", "重置 Timer 与帧索引。"))
                            ResetSpriteAnimationPlayback(component);
                    },
                    [&]()
                    {
                        if (DrawTableFillButton("Stop", "停止播放。"))
                            component.Playing = false;
                    });
            },
            [&]() { drawContext.entity.RemoveComponent<SpriteAnimationComponent>(); });
    }

    struct SpriteAnimationComponentInspectorRegistrar
    {
        SpriteAnimationComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<SpriteAnimationComponent>(
                "SpriteAnimationComponent", "Sprite Animation", "Sprite Animation", 90,
                &DrawSpriteAnimationComponentInspectorUI);
        }
    };

    static SpriteAnimationComponentInspectorRegistrar s_SpriteAnimationComponentInspectorRegistrar;
}
