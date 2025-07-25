#pragma once
#include "Engine.h"

class ExampleLayer : public Himii::Layer {
public:
    ExampleLayer();
    virtual ~ExampleLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdate() override;
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Himii::Event &event) override;

private:
    Himii::Ref<Himii::Shader> m_Shader;
    Himii::Ref<Himii::VertexArray> m_VertexArray;
    Himii::Ref<Himii::VertexArray> m_SquareVA;

    Himii::OrthographicCamera m_Camera;
};
