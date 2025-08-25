#pragma once
#include "Engine.h"
#include "imgui.h"
// 为了在头文件中保存选中实体句柄
#include <entt/entt.hpp>

namespace Himii { class Framebuffer; }

class EditorLayer : public Himii::Layer {
public:
    EditorLayer() : Himii::Layer("EditorLayer") {}
    ~EditorLayer() override = default;

    void OnAttach() override {}
    void OnDetach() override {}
    void OnUpdate(Himii::Timestep) override {}

    void OnImGuiRender() override;

    // 对外钩子：让运行层把场景纹理传入
public:
    void SetSceneTexture(uint32_t textureID) { m_SceneTexture = textureID; }
    void SetSceneSize(uint32_t w, uint32_t h) { m_SceneWidth = w; m_SceneHeight = h; }
    ImVec2 GetSceneDesiredSize() const { return m_LastSceneAvail; }
    bool   IsSceneHovered() const { return m_SceneHovered; }
    bool   IsSceneFocused() const { return m_SceneFocused; }
    // 将鼠标转换为 Scene 视口内的像素坐标（基于最近一次绘制的 Image 区域）。返回是否在范围内
    bool GetSceneMousePixel(uint32_t texW, uint32_t texH, uint32_t &outX, uint32_t &outY) const;

    void SetGameTexture(uint32_t textureID) { m_GameTexture = textureID; }
    void SetGameSize(uint32_t w, uint32_t h) { m_GameWidth = w; m_GameHeight = h; }
    ImVec2 GetGameDesiredSize() const { return m_LastGameAvail; }
    bool   IsGameHovered() const { return m_GameHovered; }
    bool   IsGameFocused() const { return m_GameFocused; }

    // 注入当前运行中的场景，供 Hierarchy/Inspector 使用
public:
    void SetActiveScene(Himii::Scene* scene) { m_ActiveScene = scene; if (!scene) m_SelectedEntity = entt::null; }
    Himii::Scene* GetActiveScene() const { return m_ActiveScene; }
    void SetSelectedEntity(entt::entity e) { m_SelectedEntity = e; }

private:
    uint32_t m_SceneTexture = 0;
    uint32_t m_SceneWidth = 0, m_SceneHeight = 0;
    ImVec2   m_LastSceneAvail = {0, 0};
    bool     m_SceneHovered = false;
    bool     m_SceneFocused = false;
    ImVec2   m_SceneImageMin = {0, 0};
    ImVec2   m_SceneImageSize = {0, 0};

    uint32_t m_GameTexture = 0;
    uint32_t m_GameWidth = 0, m_GameHeight = 0;
    ImVec2   m_LastGameAvail = {0, 0};
    bool     m_GameHovered = false;
    bool     m_GameFocused = false;

    // ECS 面板状态
    Himii::Scene* m_ActiveScene = nullptr;
    entt::entity  m_SelectedEntity{entt::null};
};
