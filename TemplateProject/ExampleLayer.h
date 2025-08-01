#pragma once
#include "Engine.h"

class ExampleLayer : public Himii::Layer {
public:
    ExampleLayer();
    virtual ~ExampleLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdate(Himii::Timestep ts) override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Himii::Event &event) override;

private:
    Himii::ShaderLibrary m_ShaderLibrary;
    Himii::Ref<Himii::Shader> m_Shader;
    Himii::Ref<Himii::VertexArray> m_VertexArray;
    Himii::Ref<Himii::VertexArray> m_SquareVA;

    Himii::Ref<Himii::Texture2D> m_Texture;

    Himii::OrthographicCameraController m_CameraController;

    glm::vec4 m_SquareColor1;
    glm::vec4 m_SquareColor2;
};
