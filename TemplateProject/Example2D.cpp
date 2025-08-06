#include "Example2D.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Example2D::Example2D() : Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f,0.38f,0.64f,1.0f})
{

}

void Example2D::OnAttach()
{
    HIMII_PROFILE_FUNCTION();

    m_BlockTexture = Himii::Texture2D::Create("assets/textures/grass.png");
    m_Terrain = Himii::CreateRef<Terrain>();

    m_Terrain->GenerateTerrain();
}
void Example2D::OnDetach()
{
    HIMII_PROFILE_FUNCTION();
}

void Example2D::OnUpdate(Himii::Timestep ts)
{
    HIMII_PROFILE_FUNCTION();

    {
        HIMII_PROFILE_SCOPE("CameraController::OnUpdate");
        m_CameraController.OnUpdate(ts);
    }

    {
        HIMII_PROFILE_SCOPE("Renderer Prep")
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();
    }

    {
        HIMII_PROFILE_SCOPE("Renderer Draw");
        Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());

       for (int y = 0; y < m_Terrain->getHeight(); ++y)
        {
            for (int x = 0; x < m_Terrain->GetWidth(); ++x)
            {
                BlockType block = m_Terrain->GetBlocks()[y][x];
                glm::vec4 color=blockColors[block];
                if (block != AIR)
                {
                    Himii::Renderer2D::DrawQuad({y, x}, {0.2, 0.2}, {1,1,1,1});
                    HIMII_CORE_INFO("color:{0},{1},{2}",color.r,color.b,color.g);
                }
            }
        }
        Himii::Renderer2D::DrawQuad({0,0}, {0.2, 0.2}, glm::vec4{0.25,0.13,0.26,1.0});
        Himii::Renderer2D::EndScene();
    }
}
void Example2D::OnImGuiRender()
{
    HIMII_PROFILE_FUNCTION();
    ImGui::Begin("设置");
    ImGui::Text("颜色设置");
    ImGui::ColorEdit4("矩形颜色", glm::value_ptr(m_SquareColor));
    ImGui::End();
}
void Example2D::OnEvent(Himii::Event& event)
{
    m_CameraController.OnEvent(event);
}
