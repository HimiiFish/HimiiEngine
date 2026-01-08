#pragma once

#include "Engine.h"
#include "Himii/Renderer/Texture.h"
#include <unordered_map>

namespace Himii
{
    class SceneHierarchyPanel {
    public:
        SceneHierarchyPanel();
        SceneHierarchyPanel(const Ref<Scene> &context);

        void SetContext(const Ref<Scene> &context);

        void OnImGuiRender();

        Entity GetSelectedEntity()
        {
            return m_SelectionContext;
        }
        void SetSelectedEntity(Entity entity)
        {
            m_SelectionContext = entity;
        }

    private:
        template<typename T>
        void DisplayAddComponentEntry(const std::string &entryName);

        void DrawEntityNode(Entity entity);
        void DrawComponents(Entity entity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;
        std::unordered_map<std::string, Ref<Texture2D>> m_ComponentIcons;
    };
}