#pragma once
#include "Engine.h"
#include "panel/SceneHierarchyPanel.h"
#include "panel/ContentBrowserPanel.h"

#include "Himii/Renderer/EditorCamera.h"

namespace Himii
{
    class EditorLayer : public Himii::Layer {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;

        virtual void OnUpdate(Himii::Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Himii::Event &event) override;

    private:
        bool OnKeyPressed(KeyPressedEvent &e);
        bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

        void NewScene();
        void OpenScene();
        void OpenScene(const std::filesystem::path& path);
        void SaveScene();
        void SaveSceneAs();

        void SerializeScene(Ref<Scene> scene, const std::filesystem::path &path);

        void OnScenePlay();
        void OnSceneStop();

        void OnDuplicateEntity();

        //UI panel
        void UI_Toolbar();
    private:
        Ref<Scene> m_ActiveScene;
        Ref<Scene> m_EditorScene;

        std::filesystem::path m_EditorScenePath;

        Entity m_SquareEntity;
        Entity m_CameraEntity;

        Entity m_HoveredEntity;

        OrthographicCameraController m_CameraController;
        EditorCamera m_EditorCamera;

        bool m_ViewportFocused = false, m_ViewportHovered = false;

        // 可选：后续可扩展纹理/渲染资源，这里最小示例仅用颜色方块

        // Scene 视口用的离屏帧缓冲
        Ref<Framebuffer> m_Framebuffer;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec2 m_ViewportBounds[2];

        glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f, 1.0f};

        int m_GizmoType = -1;

        enum class SceneState {
            Edit = 0,
            Play = 1,
            Simulate = 2
        };
        SceneHierarchyPanel m_SceneHierarchyPanel;
        ContentBrowserPanel m_ContentBrowserPanel;

        Ref<Texture2D> m_IconPlay, m_IconStop;


        SceneState m_SceneState = SceneState::Edit;
    };
}