#pragma once
#include "Engine.h"

namespace Himii
{
    class MainScene : public Layer {
    public:
        MainScene();
        virtual ~MainScene() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;

        virtual void OnUpdate(Himii::Timestep ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Himii::Event &event) override;

    private:
        Himii::Scene m_Scene;
        Himii::OrthographicCameraController m_CameraController;

        glm::vec2 m_ViewportSize = {0.0f, 0.0f};
        glm::vec4 m_SquareColor = {0.5f, 0.26f, 0.56f, 1.0f};
    };
} // namespace Himii
