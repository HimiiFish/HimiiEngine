#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Resource/AssetManager.h"
#include "Resource/Sprite.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"
#include "Project/SortingLayerSettings.h"

#include <algorithm>
#include <filesystem>

namespace Himii
{
    static void DrawSpriteRendererComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<SpriteRendererComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<SpriteRendererComponent>();
        Ref<Texture2D> icon =
            drawContext.getComponentIcon ? drawContext.getComponentIcon("Sprite Renderer") : nullptr;

        DrawComponentInspectorHeaderUI(
            drawContext, "SpriteRendererComponent", "Sprite Renderer", icon,
            [&]()
            {
                DrawColorControl("Color", component.Color);

                auto assetManager = ResourceSystem::GetAssetManager();

                Ref<Texture2D> previewTexture;
                std::string displayNameStorage = "None";
                const char* displayName = displayNameStorage.c_str();
                AssetHandle parentTextureHandle = 0;

                if (assetManager && component.SpriteAssetHandle != 0)
                {
                    const SpriteDefinition* spriteDefinition =
                        assetManager->GetSpriteDefinition(component.SpriteAssetHandle);
                    if (spriteDefinition)
                        displayNameStorage = spriteDefinition->Name;

                    parentTextureHandle =
                        assetManager->GetTextureHandleForSprite(component.SpriteAssetHandle);
                    if (parentTextureHandle != 0)
                    {
                        Ref<Asset> textureAsset = assetManager->GetAsset(parentTextureHandle);
                        if (textureAsset)
                            previewTexture = std::static_pointer_cast<Texture2D>(textureAsset);

                        const std::filesystem::path texturePath =
                            assetManager->GetAssetRegistry().at(parentTextureHandle).FilePath;
                        displayNameStorage += " (";
                        displayNameStorage += texturePath.filename().string();
                        displayNameStorage += ")";
                    }
                    displayName = displayNameStorage.c_str();
                }

                DrawObjectReferenceField(
                    "Sprite", displayName, component.SpriteAssetHandle != 0, previewTexture,
                    [&]()
                    {
                        component.SpriteAssetHandle = 0;
                    },
                    [&](const ImGuiPayload* payload)
                    {
                        return AssignSpriteFromContentBrowserPayload(payload, component.SpriteAssetHandle);
                    },
                    [&]()
                    {
                        if (parentTextureHandle != 0 && drawContext.requestTextureInspector)
                            drawContext.requestTextureInspector(parentTextureHandle);
                    });

                if (assetManager && parentTextureHandle != 0)
                {
                    const std::vector<SpriteDefinition>& sprites =
                        assetManager->GetSpritesForTexture(parentTextureHandle);

                    if (sprites.size() > 1)
                    {
                        int currentSpriteIndex = 0;
                        std::vector<const char*> spriteLabels;
                        spriteLabels.reserve(sprites.size());
                        for (int spriteIndex = 0; spriteIndex < static_cast<int>(sprites.size()); ++spriteIndex)
                        {
                            spriteLabels.push_back(sprites[spriteIndex].Name.c_str());
                            if (sprites[spriteIndex].Handle == component.SpriteAssetHandle)
                                currentSpriteIndex = spriteIndex;
                        }

                        DrawEnumComboControl(
                            "Variant", currentSpriteIndex, spriteLabels.data(),
                            static_cast<int>(spriteLabels.size()),
                            [&](int newIndex)
                            {
                                if (newIndex >= 0 && newIndex < static_cast<int>(sprites.size()))
                                    component.SpriteAssetHandle = sprites[newIndex].Handle;
                            });
                    }
                }

                DrawFloatControl("Tiling Factor", component.TilingFactor, 0.1f, 0.0f, 100.0f, nullptr,
                                 nullptr, true, 1.0f);
                DrawCheckboxControl("Flip Horizontal", component.FlipHorizontal, false);

                component.SortingLayer = std::clamp(component.SortingLayer, 0, SortingLayerCount - 1);
                int sortingLayerIndex = component.SortingLayer;

                SortingLayerSettings sortingLayerSettings;
                if (Project::GetActive())
                    sortingLayerSettings = Project::GetConfig().SortingLayers;

                std::vector<const char*> sortingLayerLabels;
                sortingLayerLabels.reserve(SortingLayerCount);
                for (int layerIndex = 0; layerIndex < SortingLayerCount; ++layerIndex)
                    sortingLayerLabels.push_back(sortingLayerSettings.LayerNames[layerIndex].c_str());

                DrawEnumComboControl(
                    "Sorting Layer", sortingLayerIndex, sortingLayerLabels.data(), SortingLayerCount,
                    [&](int newIndex) { component.SortingLayer = newIndex; });

                DrawIntControl("Sorting Order", component.SortingOrder);
            },
            [&]() { drawContext.entity.RemoveComponent<SpriteRendererComponent>(); });
    }

    struct SpriteRendererComponentInspectorRegistrar
    {
        SpriteRendererComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<SpriteRendererComponent>(
                "SpriteRendererComponent", "Sprite Renderer", "Sprite Renderer", 30,
                &DrawSpriteRendererComponentInspectorUI);
        }
    };

    static SpriteRendererComponentInspectorRegistrar s_SpriteRendererComponentInspectorRegistrar;
}
