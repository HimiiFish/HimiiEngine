//#pragma once
//#include "SceneHierarchyPanel.h"
//
//#include <imgui.h>
//#include <imgui_internal.h>
//#include <misc/cpp/imgui_stdlib.h>
//
//namespace Himii
//{
//    SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene> &context)
//    {
//        SetContext(context);
//    }
//    void SceneHierarchyPanel::SetContext(const Ref<Scene> &context)
//    {
//        m_Context = context;
//        m_SelectionContext = {};
//    }
//    void SceneHierarchyPanel::OnImGuiRender()
//    {
//        ImGui::Begin("Scene Hierarchy");
//
//        /*auto view = m_Context->Registry().group<TagComponent, TransformComponent>();
//        for ( auto e:view)
//        {
//            Entity entity{e, m_Context.get()};
//            DrawEntityNode(entity);
//        }*/
//
//        ImGui::End();
//    }
//    void SceneHierarchyPanel::DrawEntityNode(Entity entity)
//    {
//        auto &tag = entity.GetComponent<TagComponent>().name;
//
//        ImGuiTreeNodeFlags flags =( (m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
//        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
//        bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, tag.c_str());
//        if (ImGui::IsItemClicked())
//        {
//            m_SelectionContext = entity;
//        }
//        if (opened)
//        {
//            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
//            bool opened = ImGui::TreeNodeEx((void *)0, flags, tag.c_str());
//
//            if (opened)
//                ImGui::TreePop();
//            ImGui::TreePop();
//        }
//    }
//} // namespace Himii
