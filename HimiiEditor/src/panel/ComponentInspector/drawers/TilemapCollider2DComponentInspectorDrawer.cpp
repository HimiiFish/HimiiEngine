#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Resource/AssetManager.h"
#include "Project/Project.h"
#include "Resource/ResourceSystem.h"
#include "World/Scene/Components.h"
#include "Module/Tilemap/TileMapData.h"
#include "Module/Tilemap/TileSet.h"
#include "Module/Tilemap/TilemapColliderBuilder.h"

#include <imgui.h>

namespace Himii
{
    static void DrawTilemapCollider2DComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<TilemapCollider2DComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<TilemapCollider2DComponent>();

        DrawComponentInspectorHeaderUI(
            drawContext, "TilemapCollider2DComponent", "Tilemap Collider 2D", nullptr,
            [&]()
            {
                DrawCheckboxControl("Enabled", component.Enabled);
                DrawCheckboxControl("Merge Adjacent Cells", component.MergeAdjacentCells);

                if (!drawContext.entity.HasComponent<TilemapComponent>())
                    return;

                const TilemapComponent& tilemapComponent = drawContext.entity.GetComponent<TilemapComponent>();
                if (tilemapComponent.TileMapHandle == 0 || !drawContext.scene || !Project::GetActive())
                    return;

                auto assetManager = ResourceSystem::GetAssetManager();
                if (!assetManager)
                    return;

                Ref<TileMapData> mapData = std::static_pointer_cast<TileMapData>(
                    assetManager->GetAsset(tilemapComponent.TileMapHandle));
                if (!mapData || mapData->GetTileSetHandle() == 0)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f),
                                       "TileMap has no TileSet assigned.");
                    return;
                }

                Ref<TileSet> tileSet =
                    std::static_pointer_cast<TileSet>(assetManager->GetAsset(mapData->GetTileSetHandle()));
                if (!tileSet)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f),
                                       "TileSet asset could not be loaded.");
                    return;
                }

                const TilemapColliderBuildReport report =
                    TilemapColliderBuilder::AnalyzeCollidableCells(*mapData, *tileSet);

                ImGui::Separator();
                ImGui::Text("Painted cells: %u", report.paintedCellCount);
                ImGui::Text("Collidable cells (Play): %u", report.collidableCellCount);
                ImGui::Text("Unknown tile IDs: %u", report.orphanTileIdentifierCount);

                if (report.collidableCellCount == 0)
                {
                    ImGui::TextColored(
                        ImVec4(1.0f, 0.85f, 0.3f, 1.0f),
                        "No collidable cells. Open TileMap Setup, enable Collidable on"
                        " tile types, Save TileSet, then Play.");
                }
                else if (report.orphanTileIdentifierCount > 0)
                {
                    ImGui::TextColored(
                        ImVec4(1.0f, 0.85f, 0.3f, 1.0f),
                        "Some painted cells use missing tile IDs. Re-slice or repaint.");
                }
            },
            [&]() { drawContext.entity.RemoveComponent<TilemapCollider2DComponent>(); });
    }

    struct TilemapCollider2DComponentInspectorRegistrar
    {
        TilemapCollider2DComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<TilemapCollider2DComponent>(
                "TilemapCollider2DComponent", "Tilemap Collider 2D", "", 110,
                &DrawTilemapCollider2DComponentInspectorUI);
        }
    };

    static TilemapCollider2DComponentInspectorRegistrar s_TilemapCollider2DComponentInspectorRegistrar;
}
