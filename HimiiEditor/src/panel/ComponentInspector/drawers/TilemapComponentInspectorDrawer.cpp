#include "panel/ComponentInspector/ComponentInspectorDrawContext.h"
#include "panel/ComponentInspector/ComponentInspectorHeader.h"
#include "panel/ComponentInspector/ComponentInspectorRegistry.h"
#include "InspectorControls.h"

#include "Himii/Asset/AssetManager.h"
#include "Himii/Asset/AssetSerializer.h"
#include "Himii/Project/Project.h"
#include "Himii/Scene/Components.h"
#include "Himii/Scene/TileMapData.h"
#include "Himii/Scene/TileSet.h"

#include <filesystem>
#include <imgui.h>

namespace Himii
{
    static void DrawTilemapComponentInspectorUI(ComponentInspectorDrawContext& drawContext)
    {
        if (!drawContext.entity.HasComponent<TilemapComponent>())
            return;

        auto& component = drawContext.entity.GetComponent<TilemapComponent>();

        DrawComponentInspectorHeaderUI(
            drawContext, "TilemapComponent", "Tilemap", nullptr,
            [&]()
            {
                auto assetManager = Project::GetAssetManager();

                std::string tileMapDisplayName = "None";
                if (component.TileMapHandle != 0 && assetManager
                    && assetManager->IsAssetHandleValid(component.TileMapHandle))
                {
                    const auto& registry = assetManager->GetAssetRegistry();
                    const auto iterator = registry.find(component.TileMapHandle);
                    if (iterator != registry.end())
                        tileMapDisplayName = iterator->second.FilePath.filename().string();
                }

                DrawObjectReferenceField(
                    "Tile Map", tileMapDisplayName.c_str(), component.TileMapHandle != 0, nullptr,
                    [&]()
                    {
                        component.TileMapHandle = 0;
                    },
                    [&](const ImGuiPayload* payload)
                    {
                        if (!payload || !assetManager)
                            return false;

                        const wchar_t* path = (const wchar_t*)payload->Data;
                        std::filesystem::path assetPath(path);
                        if (assetPath.extension() != ".tilemap")
                            return false;

                        AssetHandle handle = assetManager->ImportAsset(assetPath);
                        if (handle == 0)
                            return false;

                        component.TileMapHandle = handle;
                        return true;
                    },
                    [&]()
                    {
                        if (component.TileMapHandle != 0 && drawContext.requestTileMapEditor)
                            drawContext.requestTileMapEditor(component.TileMapHandle);
                    });

                if (component.TileMapHandle == 0)
                {
                    DrawActionButtonRow(
                        "Tile Map",
                        [&]()
                        {
                            if (ImGui::Button("Create New TileMap", ImVec2(-1.0f, 0.0f)))
                            {
                                if (!assetManager)
                                    return;

                                const auto assetDirectory = Project::GetAssetDirectory();

                                auto tileSetAsset = std::make_shared<TileSet>();
                                tileSetAsset->Handle = AssetHandle();

                                TileAtlasSource atlasSource;
                                atlasSource.TileSize = 16;
                                tileSetAsset->AddAtlasSource(atlasSource);

                                std::filesystem::path tileSetDirectory = assetDirectory / "tilesets";
                                std::filesystem::create_directories(tileSetDirectory);
                                std::filesystem::path tileSetPath = tileSetDirectory / "default.tileset";
                                int nameIndex = 0;
                                while (std::filesystem::exists(tileSetPath))
                                    tileSetPath = tileSetDirectory
                                        / ("default_" + std::to_string(++nameIndex) + ".tileset");

                                TileSetSerializer::Serialize(tileSetPath, tileSetAsset);
                                const auto tileSetRelativePath =
                                    std::filesystem::relative(tileSetPath, assetDirectory);
                                const AssetHandle tileSetHandle =
                                    assetManager->ImportAsset(tileSetRelativePath);
                                tileSetAsset->Handle = tileSetHandle;
                                TileSetSerializer::Serialize(tileSetPath, tileSetAsset);

                                auto mapAsset = std::make_shared<TileMapData>();
                                mapAsset->Handle = AssetHandle();
                                mapAsset->SetTileSetHandle(tileSetHandle);
                                mapAsset->SetCellSize(1.0f);

                                std::filesystem::path tileMapDirectory = assetDirectory / "tilemaps";
                                std::filesystem::create_directories(tileMapDirectory);
                                std::filesystem::path tileMapPath = tileMapDirectory / "new_tilemap.tilemap";
                                nameIndex = 0;
                                while (std::filesystem::exists(tileMapPath))
                                    tileMapPath = tileMapDirectory
                                        / ("new_tilemap_" + std::to_string(++nameIndex) + ".tilemap");

                                TileMapDataSerializer::Serialize(tileMapPath, mapAsset);
                                const auto tileMapRelativePath =
                                    std::filesystem::relative(tileMapPath, assetDirectory);
                                const AssetHandle tileMapHandle =
                                    assetManager->ImportAsset(tileMapRelativePath);
                                mapAsset->Handle = tileMapHandle;
                                TileMapDataSerializer::Serialize(tileMapPath, mapAsset);

                                assetManager->SerializeAssetRegistry();

                                component.TileMapHandle = tileMapHandle;
                                if (drawContext.requestTileMapEditor)
                                    drawContext.requestTileMapEditor(tileMapHandle);
                            }
                        });
                }
                else if (assetManager && assetManager->IsAssetHandleValid(component.TileMapHandle))
                {
                    ImGui::TextDisabled(
                        "选中实体显示 Scene 网格。在 TileMap Setup 右侧点选图块后再绘制；"
                        "Move Entity [H] 可移动实体。双击引用或点名称旁按钮打开编辑器。");
                }
            },
            [&]() { drawContext.entity.RemoveComponent<TilemapComponent>(); });
    }

    struct TilemapComponentInspectorRegistrar
    {
        TilemapComponentInspectorRegistrar()
        {
            ComponentInspectorRegistry::Get().RegisterComponentInspector<TilemapComponent>(
                "TilemapComponent", "Tilemap", "", 100, &DrawTilemapComponentInspectorUI);
        }
    };

    static TilemapComponentInspectorRegistrar s_TilemapComponentInspectorRegistrar;
}
