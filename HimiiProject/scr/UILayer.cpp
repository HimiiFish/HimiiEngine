#include "UILayer.h"

UILayer::UILayer() : Layer("UILayer"), m_CameraController(1280.0f / 720.0f)
{
}

void UILayer::OnAttach()
{
    auto button = uiScene.CreateEntity("Button");
    button.AddComponent<SpriteRenderer>(glm::vec4(0.2f, 0.6f, 0.8f, 1.0f));
    auto &btn = button.AddComponent<Button>();
    btn.onClick = []() { HIMII_CORE_INFO("Button Clicked!"); };
}

void UILayer::OnDetach()
{
}

void UILayer::OnUpdate(Himii::Timestep ts)
{
    HIMII_PROFILE_FUNCTION();

    m_CameraController.OnUpdate(ts);

    RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    RenderCommand::Clear();

    HIMII_PROFILE_SCOPE("Renderer Draw");

    Renderer2D::BeginScene(m_CameraController.GetCamera());
    uiScene.OnUpdate(ts);
    Renderer2D::EndScene();
}


void UILayer::OnImGuiRender()
{
}
void UILayer::OnEvent(Event &event)
{
    m_CameraController.OnEvent(event);
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(UILayer::OnWindowResize));
    dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(UILayer::OnMouseButtonPressed));
}

bool UILayer::OnWindowResize(WindowResizeEvent &e)
{
    return false;
}

bool UILayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    return false;
}

Entity UILayer::HitTest(const glm::vec2 &uiPos) const
{
    return Entity();
}
