#pragma once
#include "Engine.h"
#include "panel/SceneHierarchyPanel.h"

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

    private:
        Ref<Scene> m_ActiveScene;

        Entity m_SquareEntity;
        Entity m_CameraEntity;

        OrthographicCameraController m_CameraController;

        // 可选：后续可扩展纹理/渲染资源，这里最小示例仅用颜色方块

        // Scene 视口用的离屏帧缓冲
        Ref<Framebuffer> m_Framebuffer;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f, 1.0f};


        SceneHierarchyPanel m_SceneHierarchyPanel;
    };
}