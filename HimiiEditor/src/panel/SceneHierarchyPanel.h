//#pragma once
//
//#include "Engine.h"
//
//namespace Himii
//{
//    class SceneHierarchyPanel {
//    public:
//        SceneHierarchyPanel() = default;
//        SceneHierarchyPanel(const Ref<Scene> &context);
//
//        void SetContext(const Ref<Scene> &context);
//
//        void OnImGuiRender();
//        Entity GetSelectedEntity()
//        {
//            return m_SelectionContext;
//        }
//        void SetSelectedEntity(Entity entity)
//        {
//            m_SelectionContext = entity;
//        }
//
//    private:
//        void DrawEntityNode(Entity entity);
//        //void DrawComponents(Entity entity);
//
//    private:
//        Ref<Scene> m_Context;
//        Entity m_SelectionContext;
//    };
//}