#pragma once
#include "Engine.h"

using namespace Himii;

class UILayer : public Layer {
public:
    UILayer();
    virtual ~UILayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdate(Himii::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Himii::Event &event) override;

private:
    bool OnWindowResize(WindowResizeEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

    Entity HitTest(const glm::vec2 &uiPos) const;

private:
    Scene uiScene;
    OrthographicCameraController m_CameraController;

    glm::vec2 m_MousePosition = {0.0f, 0.0f};
    Entity m_HoveredEntity;
};
