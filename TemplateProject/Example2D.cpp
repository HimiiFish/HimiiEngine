#include "Example2D.h"
#include "imgui.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

Example2D::Example2D() :
    Layer("Example2D"), m_CameraController(1280.0f / 720.0f), m_SquareColor({0.2f, 0.38f, 0.64f, 1.0f})
{
}

void Example2D::OnAttach()
{
    HIMII_PROFILE_FUNCTION();

    m_GrassTexture = Himii::Texture2D::Create("assets/textures/grass.png");
    m_MudTexture = Himii::Texture2D::Create("assets/textures/mud.png");

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

        Himii::Renderer2D::DrawQuad({0.5, 0.5}, {1, 1}, m_GrassTexture);
        Himii::Renderer2D::DrawQuad({-0.5, 0.5}, {1, 1}, {0.12, 0.45, 0.62, 1.0});
        // Himii::Renderer2D::DrawRotatedQuad({-0.5, -0.5}, {1.2, 1.2}, glm::radians(45.0f), {0.3, 0.56, 0.52,1.0});

//#pragma region testTerrain
//        const auto &blocks = m_Terrain->GetBlocks();
//        const float tileSize = 0.2f; // 与绘制尺寸一致
//
//        for (int y = 0; y < m_Terrain->getHeight(); ++y)
//        {
//            for (int x = 0; x < m_Terrain->GetWidth(); ++x)
//            {
//                BlockType block = blocks[y][x];
//                if (block == GRASS)
//                {
//                    int renderY = m_Terrain->getHeight() - 1 - y;
//                    Himii::Renderer2D::DrawQuad({x * tileSize, renderY * tileSize}, {tileSize, tileSize},m_GrassTexture);
//                }
//                if (block == DIRT)
//                {
//                    int renderY = m_Terrain->getHeight() - 1 - y;
//                    Himii::Renderer2D::DrawQuad({x * tileSize, renderY * tileSize}, {tileSize, tileSize},m_MudTexture);
//                }
//                else
//                {
//                    glm::vec4 color = blockColors[block];
//                    int renderY = m_Terrain->getHeight() - 1 - y;
//                    Himii::Renderer2D::DrawQuad({x * tileSize, renderY * tileSize}, {tileSize, tileSize}, color);
//                }
//            }
//        }
//#pragma endregion

        Himii::Renderer2D::DrawQuad({0, 0}, {0.2, 0.2}, glm::vec4{0.25, 0.13, 0.26, 1.0});
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

    ImGui::Begin("Debug");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}
void Example2D::OnEvent(Himii::Event &event)
{
    m_CameraController.OnEvent(event);
}
