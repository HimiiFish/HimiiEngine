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

    m_Terrain = Himii::CreateRef<Terrain>(200,40);//

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

    Himii::Renderer2D::ResetStats();
    {
        HIMII_PROFILE_SCOPE("Renderer Prep")
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();
    }
    
    {

        static float rotation = 0.0f;
        rotation += ts * 20.0f;
        HIMII_PROFILE_SCOPE("Renderer Draw");
        Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());

        // Himii::Renderer2D::DrawQuad({0.5, 0.5}, {0.5, 0.5}, m_GrassTexture);
        // Himii::Renderer2D::DrawQuad({-0.5, 0.5}, {0.5, 0.5}, {0.12, 0.45, 0.62, 1.0});
        // Himii::Renderer2D::DrawRotatedQuad({-0.5, -0.5}, {0.5, 0.5}, 45.0f, m_MudTexture, 10.0f);
        // Himii::Renderer2D::DrawRotatedQuad({0.5, -0.5}, {0.5, 0.5}, rotation, {0.25,0.56,0.56,1.0});
        // Himii::Renderer2D::DrawQuad({0, 0}, {0.2, 0.2}, glm::vec4{0.25, 0.13, 0.26, 1.0});

#pragma region testTerrain
       const auto &blocks = m_Terrain->GetBlocks();
       const float tileSize = 0.2f;

        //使用 m_Terrain->getHeight() 和 m_Terrain->GetWidth() 确保渲染尺寸正确
       for (int y = 0; y < m_Terrain->getHeight(); ++y)
       {
           for (int x = 0; x < m_Terrain->GetWidth(); ++x)
           {
               BlockType block = blocks[y][x];
               if (block != AIR) // 只绘制非空气区块
               {
                   Himii::Renderer2D::DrawQuad({x * tileSize, y * tileSize}, {tileSize, tileSize}, blockColors[block]);
               }
           }
       }
#pragma endregion

        Himii::Renderer2D::EndScene();

        /*Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());
        for (float y = -5.0f; y < 5.0f; y += 0.5f)
        {
            for (float x = -5.0f; x < 5.0f; x += 0.5f)
            {
                glm::vec4 color = {(x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f};
                Himii::Renderer2D::DrawQuad({x, y}, {0.45f, 0.45f}, color);
            }
        }
        Himii::Renderer2D::EndScene();*/
    }
}
void Example2D::OnImGuiRender()
{
    HIMII_PROFILE_FUNCTION();
    ImGui::Begin("设置");
    ImGui::Text("颜色设置");
    ImGui::ColorEdit4("矩形颜色", glm::value_ptr(m_SquareColor));
    ImGui::Text("Renderer2D Stats");
    ImGui::Text("Draw Calls: %d", Himii::Renderer2D::GetStatistics().DrawCalls);
    ImGui::Text("Quad Count: %d", Himii::Renderer2D::GetStatistics().QuadCount);
    ImGui::Text("Total Vertex Count: %d", Himii::Renderer2D::GetStatistics().GetTotalVertexCount());
    ImGui::Text("Total Index Count: %d", Himii::Renderer2D::GetStatistics().GetTotalIndexCount());
    ImGui::End();

    ImGui::Begin("Debug");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}
void Example2D::OnEvent(Himii::Event &event)
{
    m_CameraController.OnEvent(event);
}
