#pragma once

#include "Engine.h"
#include "Module/Render/RenderCore/Texture.h"

#include <filesystem>
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

        std::filesystem::path GetAnimationEditorRequest()
        {
            std::filesystem::path path = m_AnimationEditorRequest;
            m_AnimationEditorRequest.clear();
            return path;
        }

    private:
        template<typename T>
        void DisplayAddComponentEntry(const std::string &entryName);

        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);
        void DrawHierarchyRoots(bool userInterfaceEntities);
        void HandleEntityReparent(Entity draggedEntity, Entity newParentEntity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
        std::unordered_map<std::string, Ref<Texture2D>> m_ComponentIcons;
        AssetHandle m_TileMapEditorRequest = 0;
        AssetHandle m_ParticleEmitterEditorRequest = 0;
        AssetHandle m_TextureInspectorRequest = 0;
        std::filesystem::path m_AnimationEditorRequest;

        EditorCommandHistory* m_CommandHistory = nullptr;

        std::string m_TagEditStartValue;
    };
}