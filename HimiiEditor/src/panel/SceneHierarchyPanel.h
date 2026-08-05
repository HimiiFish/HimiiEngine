#pragma once

#include "Engine.h"
#include "Module/Render/RenderCore/Texture.h"

#include <array>
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace Himii
{
    class EditorCommandHistory;

    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel();
        SceneHierarchyPanel(const Ref<Scene> &context);

        void SetContext(const Ref<Scene> &context);
        void SetCommandHistory(EditorCommandHistory* commandHistory);

        void OnImGuiRender();

        Entity GetSelectedEntity()
        {
            return m_SelectionContext;
        }
        void SetSelectedEntity(Entity entity)
        {
            m_SelectionContext = entity;
        }

        AssetHandle GetTileMapEditorRequest()
        {
            AssetHandle h = m_TileMapEditorRequest;
            m_TileMapEditorRequest = 0;
            return h;
        }

        AssetHandle GetParticleEmitterEditorRequest()
        {
            AssetHandle h = m_ParticleEmitterEditorRequest;
            m_ParticleEmitterEditorRequest = 0;
            return h;
        }

        AssetHandle GetTextureInspectorRequest()
        {
            AssetHandle handle = m_TextureInspectorRequest;
            m_TextureInspectorRequest = 0;
            return handle;
        }

        AssetHandle GetMaterialEditorRequest()
        {
            AssetHandle handle = m_MaterialEditorRequest;
            m_MaterialEditorRequest = 0;
            return handle;
        }

        std::filesystem::path GetAnimationEditorRequest()
        {
            std::filesystem::path path = m_AnimationEditorRequest;
            m_AnimationEditorRequest.clear();
            return path;
        }

    private:
        template<typename T>
        void DisplayAddComponentEntry(const std::string &entryName, const std::string &searchFilter = {});

        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);
        void DrawHierarchyRoots(bool userInterfaceEntities);
        void HandleEntityReparent(Entity draggedEntity, Entity newParentEntity);
        void DrawCreateEntityMenu(Entity parentEntity = {});
        void DrawAddComponentMenu(bool useGroupedMenus, const std::string &searchFilter = {});
        void CreateEntityFromMenu(
                const std::string &baseName,
                const std::function<Entity(const Ref<Scene>&, const std::string&)> &createEntityFunction,
                Entity parentEntity = {});
        std::string BuildUniqueSiblingName(const std::string &baseName, Entity parentEntity,
                                           bool userInterfaceEntity) const;

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
        std::unordered_map<std::string, Ref<Texture2D>> m_ComponentIcons;
        AssetHandle m_TileMapEditorRequest = 0;
        AssetHandle m_ParticleEmitterEditorRequest = 0;
        AssetHandle m_TextureInspectorRequest = 0;
        AssetHandle m_MaterialEditorRequest = 0;
        std::filesystem::path m_AnimationEditorRequest;

        EditorCommandHistory* m_CommandHistory = nullptr;

        std::string m_TagEditStartValue;
        std::array<char, 128> m_AddComponentSearchBuffer{};
    };
}