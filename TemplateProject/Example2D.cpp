#include "Example2D.h"
#include "imgui.h"
#include "EditorLayer.h"

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
    // 加载图集（blocks.png）
    m_BlocksAtlas = Himii::Texture2D::Create("assets/textures/blocks.png");

    // 创建离屏帧缓冲，尺寸先用窗口大小，后续由 EditorLayer 面板驱动调整
    auto &app = Himii::Application::Get();
    Himii::FramebufferSpecification fbSpec{app.GetWindow().GetWidth(), app.GetWindow().GetHeight()};
    m_SceneFramebuffer = Himii::Framebuffer::Create(fbSpec);

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

    // 和 CubeLayer 一样，从 EditorLayer 获取 Scene 面板的期望尺寸并驱动 FBO 调整
    Himii::Renderer2D::ResetStats();
    EditorLayer *editorRef = nullptr;
    for (auto *layer : Himii::Application::Get().GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer))) break;
    bool resized = false;
    if (editorRef && m_SceneFramebuffer)
    {
        ImVec2 desired = editorRef->GetSceneDesiredSize();
        uint32_t newW = (uint32_t)std::max(1.0f, desired.x);
        uint32_t newH = (uint32_t)std::max(1.0f, desired.y);
        auto &spec = m_SceneFramebuffer->GetSpecification();
        if (newW != spec.Width || newH != spec.Height)
        {
            m_SceneFramebuffer->Resize(newW, newH);
            // 同步相机投影，保持 1:1 比例不随视口拉伸而变形
            m_CameraController.OnResize((float)newW, (float)newH);
            resized = true;
        }
    }
    // 渲染到离屏 FBO
    if (m_SceneFramebuffer)
    {
        m_SceneFramebuffer->Bind();
        Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
        Himii::RenderCommand::Clear();
    }
    
    {

        static float rotation = 0.0f;
        rotation += ts * 20.0f;
        HIMII_PROFILE_SCOPE("Renderer Draw");
    Himii::Renderer2D::BeginScene(m_CameraController.GetCamera());

        // 1) 基础：直接画整张纹理
        // Himii::Renderer2D::DrawQuad({-1.5f, 0.8f}, {0.6f, 0.6f}, m_BlocksAtlas);

        // 2) 图集切片演示：通过 Texture2D::GetUVFromGrid 获取四个 UV 顶点
        // 约定：左下角为 (0,0)，列向右+，行向上+
        if (m_BlocksAtlas)
        {
            auto uvGrassTop   = m_BlocksAtlas->GetUVFromGrid(2, 12, m_AtlasCols, m_AtlasRows, m_AtlasPad);
            auto uvGrassBot   = m_BlocksAtlas->GetUVFromGrid(1, 14, m_AtlasCols, m_AtlasRows, m_AtlasPad);
            auto uvDirt       = m_BlocksAtlas->GetUVFromGrid(1, 13, m_AtlasCols, m_AtlasRows, m_AtlasPad);
            auto uvStone      = m_BlocksAtlas->GetUVFromGrid(7, 8,  m_AtlasCols, m_AtlasRows, m_AtlasPad);

            float s = 0.25f; // 片大小
            // 使用自定义 UV 画出 5 张贴图，依次是草-顶/侧/底、泥土、石头
            Himii::Renderer2D::DrawQuadUV({-1.5f, 0.8f}, {s, s}, m_BlocksAtlas, uvGrassTop);
            Himii::Renderer2D::DrawQuadUV({-0.7f, 0.8f}, {s, s}, m_BlocksAtlas, uvGrassBot);
            Himii::Renderer2D::DrawQuadUV({-0.3f, 0.8f}, {s, s}, m_BlocksAtlas, uvDirt);
            Himii::Renderer2D::DrawQuadUV({ 0.1f, 0.8f}, {s, s}, m_BlocksAtlas, uvStone);
        }

#pragma region testTerrain (可见区域裁剪)
        const auto &blocks = m_Terrain->GetBlocks();
        const float tileSize = 0.2f;

        // 基于正交相机计算当前可见矩形，以减少绘制数量
        uint32_t fbW = m_SceneFramebuffer ? m_SceneFramebuffer->GetSpecification().Width : 1280;
        uint32_t fbH = m_SceneFramebuffer ? m_SceneFramebuffer->GetSpecification().Height : 720;
        float aspect = fbH > 0 ? (float)fbW / (float)fbH : (1280.0f/720.0f);
        float zoom = m_CameraController.GetZoomLevel();
        glm::vec3 camPos = m_CameraController.GetCamera().GetPosition();
        float halfW = aspect * zoom;
        float halfH = zoom;
        float minX = camPos.x - halfW;
        float maxX = camPos.x + halfW;
        float minY = camPos.y - halfH;
        float maxY = camPos.y + halfH;

        int x0 = std::max(0, (int)std::floor(minX / tileSize));
        int x1 = std::min(m_Terrain->GetWidth() - 1, (int)std::ceil(maxX / tileSize));
        int y0 = std::max(0, (int)std::floor(minY / tileSize));
        int y1 = std::min(m_Terrain->getHeight() - 1, (int)std::ceil(maxY / tileSize));

        for (int y = y0; y <= y1; ++y)
        {
            for (int x = x0; x <= x1; ++x)
            {
                BlockType block = blocks[y][x];
                if (block != AIR)
                    Himii::Renderer2D::DrawQuad({x * tileSize, y * tileSize}, {tileSize, tileSize}, blockColors[block]);
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

    // 解绑 FBO 并把纹理与尺寸交给 EditorLayer 展示
    if (m_SceneFramebuffer)
    {
        m_SceneFramebuffer->Unbind();
        if (editorRef)
        {
            editorRef->SetSceneTexture(m_SceneFramebuffer->GetColorAttachmentRendererID());
            editorRef->SetSceneSize(m_SceneFramebuffer->GetSpecification().Width,
                                    m_SceneFramebuffer->GetSpecification().Height);
        }
    }
}
void Example2D::OnImGuiRender()
{
    HIMII_PROFILE_FUNCTION();
    ImGui::Begin("设置");
    ImGui::Text("颜色设置");
    ImGui::ColorEdit4("矩形颜色", glm::value_ptr(m_SquareColor));
    ImGui::Checkbox("使用 TileMap 模式", &m_UseTileMap);
    ImGui::Text("Renderer2D Stats");
    ImGui::Text("Draw Calls: %d", Himii::Renderer2D::GetStatistics().DrawCalls);
    ImGui::Text("Quad Count: %d", Himii::Renderer2D::GetStatistics().QuadCount);
    ImGui::Text("Total Vertex Count: %d", Himii::Renderer2D::GetStatistics().GetTotalVertexCount());
    ImGui::Text("Total Index Count: %d", Himii::Renderer2D::GetStatistics().GetTotalIndexCount());
    ImGui::End();

    ImGui::Begin("Debug");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SeparatorText("Atlas (blocks.png)");
    ImGui::DragInt("Cols", &m_AtlasCols, 1, 1, 64);
    ImGui::DragInt("Rows", &m_AtlasRows, 1, 1, 64);
    ImGui::DragFloat("Padding (norm)", &m_AtlasPad, 0.0005f, 0.0f, 0.05f);
    ImGui::End();
}

// 用 blocks.png 构造一个简单 TileMap（不同 ID 使用不同图集格子）
void Example2D::DrawTileMapDemo()
{
    if (!m_BlocksAtlas) return;
    const int cols = 32;
    const int rows = 18;
    static std::vector<int> map(cols * rows, 0);
    // 简单按行生成一些花纹
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
            map[y*cols + x] = (x/4 + y/3) % 3; // 0/1/2 轮换

    auto uv0 = m_BlocksAtlas->GetUVFromGrid(2, 12, m_AtlasCols, m_AtlasRows, m_AtlasPad); // grass top
    auto uv1 = m_BlocksAtlas->GetUVFromGrid(1, 13, m_AtlasCols, m_AtlasRows, m_AtlasPad); // dirt
    auto uv2 = m_BlocksAtlas->GetUVFromGrid(7, 8,  m_AtlasCols, m_AtlasRows, m_AtlasPad); // stone
    const std::array<std::array<glm::vec2,4>,3> uvs{uv0, uv1, uv2};

    const float ts = 0.2f;
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            int id = map[y*cols + x];
            Himii::Renderer2D::DrawQuadUV({x*ts, y*ts}, {ts, ts}, m_BlocksAtlas, uvs[id]);
        }
}
void Example2D::OnEvent(Himii::Event &event)
{
    // 仅当 Scene 视口被悬停或聚焦时才处理滚轮/键盘等事件
    EditorLayer *editorRef = nullptr;
    for (auto *layer : Himii::Application::Get().GetLayerStack())
        if ((editorRef = dynamic_cast<EditorLayer *>(layer))) break;
    if (!editorRef || editorRef->IsSceneHovered() || editorRef->IsSceneFocused())
        m_CameraController.OnEvent(event);
}
