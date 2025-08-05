#include "Example2D.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Example2D::Example2D() : Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f,0.38f,0.64f,1.0f})
{

}

void Example2D::OnAttach()
{
    m_BlockTexture = Himii::Texture2D::Create("assets/textures/grass.png");
}
void Example2D::OnDetach()
{

}

void Example2D::OnUpdate(Himii::Timestep ts)
{
    m_CameraController.OnUpdate(ts);

    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());

    Himii::Renderer2D::DrawQuad({0.0, 0.0f}, {0.2f, 0.2f}, {0.15f, 0.78f, 0.45f, 1.0f});
    for (int i = 0;i < 5;i++)
    {
        Himii::Renderer2D::DrawQuad({0.6*i, 0.5f}, {0.6f, 0.6f}, m_BlockTexture);
    }
    Himii::Renderer2D::DrawQuad({0.0f, 0.0f,-0.1f}, {10.0f, 10.0f}, {0.65f, 0.42f, 0.25f,1.0f});

    Himii::Renderer2D::EndScene();
}
void Example2D::OnImGuiRender()
{
    ImGui::Begin("设置");
    ImGui::Text("颜色设置");
    ImGui::ColorEdit4("矩形颜色", glm::value_ptr(m_SquareColor));
    ImGui::End();
}
void Example2D::OnEvent(Himii::Event& event)
{
    m_CameraController.OnEvent(event);
}
