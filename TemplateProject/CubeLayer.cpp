#include "CubeLayer.h"
#include <array>
#include <algorithm>
#include "Himii/Events/ApplicationEvent.h"
#include "Himii/Core/Log.h"
#include "Himii/Renderer/RenderCommand.h"
#include "Terrain.h"
#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

CubeLayer::CubeLayer() : Himii::Layer("CubeLayer")
{
}

glm::vec4 CubeLayer::AtlasUVRect(int col, int row, int cols, int rows, float padding)
{
    // 注意坐标系：这里默认贴图原点在左下（常见做法是stbi翻转），col/row 从左下角开始计数
    const float du = 1.0f / cols;
    const float dv = 1.0f / rows;

    float u0 = col * du + padding;
    float v0 = row * dv + padding;
    float u1 = (col + 1) * du - padding;
    float v1 = (row + 1) * dv - padding;
    return {u0, v0, u1, v1};
}

// 像素矩形模式已移除，保留网格模式接口

void CubeLayer::OnAttach()
{
    // 开启深度测试（如全局已开启可省略）
    glEnable(GL_DEPTH_TEST);
    // 背面剔除：为排查可见性问题，先禁用（确认正常后可再开启并校正三角形绕序）
    glDisable(GL_CULL_FACE);

    // 1) 顶点数据：准备地形网格 VA（BuildTerrainMesh 内部会创建并填充 VB/IB）
    m_TerrainVA = Himii::VertexArray::Create();

    // 2) 地形索引缓冲稍后由 BuildTerrainMesh 生成

    // 3) 载入贴图、着色器（沿用 Texture.glsl）
    m_Atlas = Himii::Texture2D::Create("assets/textures/blocks.png");
    // 如需 Pixel 模式可在此读取像素尺寸：m_Atlas->GetWidth()/GetHeight()
    m_TextureShader = m_ShaderLibrary.Load("assets/shaders/Texture.glsl");
    m_TextureShader->Bind();
    // 设置采样器数组 [0..31]，确保 v_TexIndex=0 时能采样到纹理槽0
    int samplers[32];
    for (int i = 0; i < 32; ++i)
        samplers[i] = i;
    m_TextureShader->SetIntArray("u_Texture", samplers, 32);

    // 4) 基于柏林噪声生成 30x30x10 的体素地形网格
    BuildTerrainMesh();

    // 5) 设置一个能看见地形的默认相机位置与朝向（避免出生在方块内部）
    m_CamPos = { m_TerrainW * 0.5f, m_TerrainH * 1.2f, m_TerrainD + m_TerrainH * 1.0f };
    m_CamTarget = { m_TerrainW * 0.5f, m_TerrainH * 0.5f, m_TerrainD * 0.5f };
    m_OrientInitialized = false; // 让 UpdateCamera 基于新的目标重算 yaw/pitch
}

void CubeLayer::RebuildVB()
{
    // 6个面使用网格模式 UV（示例：草方块的 side/top/bottom）
    glm::vec4 uvFront = AtlasUVRect(m_GrassUV.side.col, m_GrassUV.side.row, m_AtlasCols, m_AtlasRows, m_GridPadding);
    glm::vec4 uvBack = uvFront;
    glm::vec4 uvLeft = uvFront;
    glm::vec4 uvRight = uvFront;
    glm::vec4 uvTop = AtlasUVRect(m_GrassUV.top.col, m_GrassUV.top.row, m_AtlasCols, m_AtlasRows, m_GridPadding);
    glm::vec4 uvBottom =
            AtlasUVRect(m_GrassUV.bottom.col, m_GrassUV.bottom.row, m_AtlasCols, m_AtlasRows, m_GridPadding);

    auto quadUV = [](const glm::vec4 &r) -> std::array<glm::vec2, 4>
    {
        // (u0,v0)=左下, (u1,v1)=右上
        return {glm::vec2{r.x, r.y}, glm::vec2{r.z, r.y}, glm::vec2{r.z, r.w}, glm::vec2{r.x, r.w}};
    };

    auto uvF = quadUV(uvFront);
    auto uvB = quadUV(uvBack);
    auto uvL = quadUV(uvLeft);
    auto uvR = quadUV(uvRight);
    auto uvT = quadUV(uvTop);
    auto uvD = quadUV(uvBottom);

    std::vector<float> vertices;
    vertices.reserve(24 * 11);

    const float s = 0.5f;
    glm::vec3 f[4] = {{-s, -s, s}, {s, -s, s}, {s, s, s}, {-s, s, s}};
    glm::vec3 b[4] = {{s, -s, -s}, {-s, -s, -s}, {-s, s, -s}, {s, s, -s}};
    glm::vec3 l[4] = {{-s, -s, -s}, {-s, -s, s}, {-s, s, s}, {-s, s, -s}};
    glm::vec3 r[4] = {{s, -s, s}, {s, -s, -s}, {s, s, -s}, {s, s, s}};
    glm::vec3 t[4] = {{-s, s, s}, {s, s, s}, {s, s, -s}, {-s, s, -s}};
    glm::vec3 d[4] = {{-s, -s, -s}, {s, -s, -s}, {s, -s, s}, {-s, -s, s}};

    auto pushFace = [&](glm::vec3 v[4], std::array<glm::vec2, 4> &uvs)
    {
        const float rC = 1.0f, gC = 1.0f, bC = 1.0f, aC = 1.0f;
        const float texIndex = 0.0f;
        const float tiling = 1.0f;
        for (int i = 0; i < 4; ++i)
        {
            vertices.push_back(v[i].x);
            vertices.push_back(v[i].y);
            vertices.push_back(v[i].z);
            vertices.push_back(rC);
            vertices.push_back(gC);
            vertices.push_back(bC);
            vertices.push_back(aC);
            vertices.push_back(uvs[i].x);
            vertices.push_back(uvs[i].y);
            vertices.push_back(texIndex);
            vertices.push_back(tiling);
        }
    };

    pushFace(f, uvF);
    pushFace(b, uvB);
    pushFace(l, uvL);
    pushFace(r, uvR);
    pushFace(t, uvT);
    pushFace(d, uvD);

    m_CubeVA->GetVertexBuffers()[0]->SetData(vertices.data(), (uint32_t)(vertices.size() * sizeof(float)));
}

void CubeLayer::OnDetach()
{
    // 这里不强制关闭，由更高层统一管理；如需关闭可恢复：glDisable(GL_DEPTH_TEST);
}

void CubeLayer::OnUpdate(Himii::Timestep ts)
{
    // 渲染准备：清颜色与深度缓冲
    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    // 相机输入更新
    UpdateCamera(ts);

    // 停止旋转，保持每个方块原位
    glm::mat4 transform(1.0f);

    // 计算透视投影与视图矩阵
    const float fovYRad = glm::radians(m_FovYDeg);
    glm::mat4 proj = glm::perspective(fovYRad, m_Aspect, m_NearZ, m_FarZ);
    // GLM 的透视默认生成的矩阵期望 NDC 的 Y 向上，Z in [-1,1]（OpenGL），无需倒置 Y
    glm::mat4 view = glm::lookAt(m_CamPos, m_CamTarget, m_CamUp);
    glm::mat4 viewProj = proj * view;

    // 提交绘制
    Himii::Renderer::BeginScene(viewProj);
    m_Atlas->Bind(0);
    if (m_TerrainVA)
        Himii::Renderer::Submit(m_TextureShader, m_TerrainVA, transform);
    Himii::Renderer::EndScene();
}

#if 0 // disabled broken duplicate definition
    void CubeLayer::OnImGuiRender()
{
    ImGui::Begin("Cube Settings");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SeparatorText("Camera (Perspective)");
    ImGui::DragFloat("FovY (deg)", &m_FovYDeg, 0.1f, 10.0f, 120.0f);
    ImGui::DragFloat("NearZ", &m_NearZ, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("FarZ", &m_FarZ, 1.0f, 10.0f, 1000.0f);
    ImGui::DragFloat3("Cam Pos", &m_CamPos.x, 0.05f);
    ImGui::DragFloat3("Cam Target", &m_CamTarget.x, 0.05f);
    ImGui::DragFloat3("Cam Up", &m_CamUp.x, 0.05f);
    ImGui::DragFloat("Aspect", &m_Aspect, 0.01f, 0.1f, 5.0f);
    ImGui::Checkbox("Mouse Look (hold RMB)", &m_MouseLook);
    ImGui::DragFloat("Move Speed", &m_MoveSpeed, 0.1f, 0.1f, 50.0f);
    ImGui::DragFloat("Mouse Sensitivity (deg/px)", &m_MouseSensitivity, 0.01f, 0.01f, 2.0f);

    ImGui::SeparatorText("Atlas (Grid)");
    // broken block removed
}

#endif
void CubeLayer::OnEvent(Himii::Event &e)
{
    using namespace Himii;
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([&](WindowResizeEvent &ev) {
        if (ev.GetHeight() > 0)
            m_Aspect = (float)ev.GetWidth() / (float)ev.GetHeight();
        return false;
    });
}
void CubeLayer::UpdateCamera(Himii::Timestep ts)
{
    using namespace Himii;
    // 初始化一次 yaw/pitch 以匹配当前 CamTarget
    if (!m_OrientInitialized)
    {
        glm::vec3 dir = glm::normalize(m_CamTarget - m_CamPos);
        // yaw: -Z 为 0? 我们以 -Z 为前方，初始 m_YawDeg=-90 指向 -Z
        m_PitchDeg = glm::degrees(asinf(glm::clamp(dir.y, -1.0f, 1.0f)));
        m_YawDeg = glm::degrees(atan2f(dir.z, dir.x));
        m_OrientInitialized = true;
        // 初始化鼠标位置
        glm::vec2 m = Input::GetMousePosition();
        m_LastMouseX = m.x;
        m_LastMouseY = m.y;
    }

    // 鼠标右键按住时开启鼠标观察
    if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
    {
        glm::vec2 cur = Input::GetMousePosition();
        float dx = cur.x - m_LastMouseX;
        float dy = cur.y - m_LastMouseY;
        m_LastMouseX = cur.x;
        m_LastMouseY = cur.y;

        m_YawDeg += dx * m_MouseSensitivity;
        m_PitchDeg -= dy * m_MouseSensitivity; // 屏幕向下为正，俯仰取负
        m_PitchDeg = glm::clamp(m_PitchDeg, -89.0f, 89.0f);
    }
    else
    {
        // 未按住时，仅更新基准，上一次位置对齐以避免下次跳变
        glm::vec2 cur = Input::GetMousePosition();
        m_LastMouseX = cur.x;
        m_LastMouseY = cur.y;
    }

    // 根据 yaw/pitch 计算前向向量（右手坐标，OpenGL 风格）
    float yawR = glm::radians(m_YawDeg);
    float pitchR = glm::radians(m_PitchDeg);
    glm::vec3 front;
    front.x = cosf(pitchR) * cosf(yawR);
    front.y = sinf(pitchR);
    front.z = cosf(pitchR) * sinf(yawR);
    front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, front));

    // 键盘 WASD 移动
    float dt = (float)ts;
    float speed = m_MoveSpeed;
    if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
        speed *= 2.0f;

    if (Input::IsKeyPressed(Key::W))
        m_CamPos += front * speed * dt;
    if (Input::IsKeyPressed(Key::S))
        m_CamPos -= front * speed * dt;
    if (Input::IsKeyPressed(Key::A))
        m_CamPos -= right * speed * dt;
    if (Input::IsKeyPressed(Key::D))
        m_CamPos += right * speed * dt;
    if (Input::IsKeyPressed(Key::Q))
        m_CamPos -= up * speed * dt;
    if (Input::IsKeyPressed(Key::E))
        m_CamPos += up * speed * dt;

    // 更新 Target 与 Up
    m_CamTarget = m_CamPos + front;
    m_CamUp = up;
}

void CubeLayer::OnImGuiRender()
{
    ImGui::Begin("Cube Settings");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SeparatorText("Camera (Perspective)");
    ImGui::DragFloat("FovY (deg)", &m_FovYDeg, 0.1f, 10.0f, 120.0f);
    ImGui::DragFloat("NearZ", &m_NearZ, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("FarZ", &m_FarZ, 1.0f, 10.0f, 1000.0f);
    ImGui::DragFloat3("Cam Pos", &m_CamPos.x, 0.05f);
    ImGui::DragFloat3("Cam Target", &m_CamTarget.x, 0.05f);
    ImGui::DragFloat3("Cam Up", &m_CamUp.x, 0.05f);
    ImGui::DragFloat("Aspect", &m_Aspect, 0.01f, 0.1f, 5.0f);
    ImGui::Checkbox("Mouse Look (hold RMB)", &m_MouseLook);
    ImGui::DragFloat("Move Speed", &m_MoveSpeed, 0.1f, 0.1f, 50.0f);
    ImGui::DragFloat("Mouse Sensitivity (deg/px)", &m_MouseSensitivity, 0.01f, 0.01f, 2.0f);

    ImGui::SeparatorText("Atlas (Grid)");
    bool atlasChanged = false;
    atlasChanged |= ImGui::DragInt("Atlas Cols", &m_AtlasCols, 1, 1, 64);
    atlasChanged |= ImGui::DragInt("Atlas Rows", &m_AtlasRows, 1, 1, 64);
    atlasChanged |= ImGui::DragFloat("Grid Padding", &m_GridPadding, 0.0005f, 0.0f, 0.05f);

    ImGui::SeparatorText("Block UV Mapping (col,row)");
    ImGui::TextUnformatted("Grass");
    atlasChanged |= ImGui::DragInt2("Grass Top", &m_GrassUV.top.col);
    atlasChanged |= ImGui::DragInt2("Grass Side", &m_GrassUV.side.col);
    atlasChanged |= ImGui::DragInt2("Grass Bottom", &m_GrassUV.bottom.col);
    ImGui::Separator();
    ImGui::TextUnformatted("Dirt");
    atlasChanged |= ImGui::DragInt2("Dirt Top", &m_DirtUV.top.col);
    atlasChanged |= ImGui::DragInt2("Dirt Side", &m_DirtUV.side.col);
    atlasChanged |= ImGui::DragInt2("Dirt Bottom", &m_DirtUV.bottom.col);
    ImGui::Separator();
    ImGui::TextUnformatted("Stone");
    atlasChanged |= ImGui::DragInt2("Stone Top", &m_StoneUV.top.col);
    atlasChanged |= ImGui::DragInt2("Stone Side", &m_StoneUV.side.col);
    atlasChanged |= ImGui::DragInt2("Stone Bottom", &m_StoneUV.bottom.col);

    if (ImGui::Button("重建地形 (应用上述 UV)") || atlasChanged)
    {
        BuildTerrainMesh();
    }
    ImGui::SeparatorText("Terrain Size");
    static int w = m_TerrainW, d = m_TerrainD, h = m_TerrainH;
    bool sizeChanged = false;
    sizeChanged |= ImGui::DragInt("Width", &w, 1, 8, 512);
    sizeChanged |= ImGui::DragInt("Depth", &d, 1, 8, 512);
    sizeChanged |= ImGui::DragInt("Height", &h, 1, 8, 256);
    if (sizeChanged)
    {
        w = std::max(8, std::min(512, w));
        d = std::max(8, std::min(512, d));
        h = std::max(8, std::min(256, h));
    }
    if (ImGui::Button("应用尺寸并重建"))
    {
        m_TerrainW = w; m_TerrainD = d; m_TerrainH = h;
        BuildTerrainMesh();
    }

    ImGui::Checkbox("参数修改后自动重建", &m_AutoRebuild);
    ImGui::SeparatorText("Noise (Perlin fBm/Ridged/Warp)");
    bool nChanged = false;
    nChanged |= ImGui::DragScalar("Seed", ImGuiDataType_U32, &m_Noise.seed, 1.0f);
    nChanged |= ImGui::DragFloat("Biome Scale", &m_Noise.biomeScale, 0.001f, 0.001f, 1.0f);
    nChanged |= ImGui::DragFloat("Continent Scale", &m_Noise.continentScale, 0.0005f, 0.002f, 0.05f);
    nChanged |= ImGui::DragFloat("Continent Strength", &m_Noise.continentStrength, 0.01f, 0.0f, 1.0f);

    ImGui::TextUnformatted("Plains (fBm)");
    nChanged |= ImGui::DragFloat("Plains Scale", &m_Noise.plainsScale, 0.001f, 0.001f, 1.0f);
    nChanged |= ImGui::DragInt("Plains Octaves", &m_Noise.plainsOctaves, 1.0f, 1, 12);
    nChanged |= ImGui::DragFloat("Plains Lacunarity", &m_Noise.plainsLacunarity, 0.01f, 1.0f, 4.0f);
    nChanged |= ImGui::DragFloat("Plains Gain", &m_Noise.plainsGain, 0.01f, 0.1f, 0.9f);

    ImGui::TextUnformatted("Mountain (Ridged)");
    nChanged |= ImGui::DragFloat("Mountain Scale", &m_Noise.mountainScale, 0.001f, 0.001f, 1.0f);
    nChanged |= ImGui::DragInt("Mountain Octaves", &m_Noise.mountainOctaves, 1.0f, 1, 12);
    nChanged |= ImGui::DragFloat("Mountain Lacunarity", &m_Noise.mountainLacunarity, 0.01f, 1.0f, 4.0f);
    nChanged |= ImGui::DragFloat("Mountain Gain", &m_Noise.mountainGain, 0.01f, 0.1f, 0.9f);
    nChanged |= ImGui::DragFloat("Ridge Sharpness", &m_Noise.ridgeSharpness, 0.01f, 0.5f, 3.0f);

    ImGui::TextUnformatted("Warp");
    nChanged |= ImGui::DragFloat("Warp Scale", &m_Noise.warpScale, 0.001f, 0.0f, 1.0f);
    nChanged |= ImGui::DragFloat("Warp Amp", &m_Noise.warpAmp, 0.01f, 0.0f, 10.0f);
    ImGui::TextUnformatted("Detail");
    nChanged |= ImGui::DragFloat("Detail Scale", &m_Noise.detailScale, 0.001f, 0.02f, 1.0f);
    nChanged |= ImGui::DragFloat("Detail Amp", &m_Noise.detailAmp, 0.01f, 0.0f, 0.5f);

    ImGui::TextUnformatted("Shape");
    nChanged |= ImGui::DragFloat("Height Mul", &m_Noise.heightMul, 0.01f, 0.1f, 2.0f);
    nChanged |= ImGui::DragFloat("Plateau", &m_Noise.plateau, 0.01f, 0.0f, 1.0f);
    nChanged |= ImGui::DragInt("Step Levels", &m_Noise.stepLevels, 1.0f, 1, 12);
    nChanged |= ImGui::DragFloat("Curve Exponent", &m_Noise.curveExponent, 0.01f, 0.5f, 2.0f);
    nChanged |= ImGui::DragFloat("Valley Depth", &m_Noise.valleyDepth, 0.01f, 0.0f, 0.4f);
    nChanged |= ImGui::DragFloat("Sea Level", &m_Noise.seaLevel, 0.01f, 0.0f, 0.9f);
    nChanged |= ImGui::DragFloat("Mountain Weight", &m_Noise.mountainWeight, 0.01f, 0.0f, 1.0f);
    if (nChanged && m_AutoRebuild)
        BuildTerrainMesh();
    ImGui::End();
}

void CubeLayer::BuildTerrainMesh()
{
    // 为避免缓冲区尺寸不足导致 glBufferSubData 失败，这里直接重建 VA/VB/IB
    m_TerrainVA = Himii::VertexArray::Create();

    PerlinNoise perlin(m_Noise.seed);
    const int W = m_TerrainW, D = m_TerrainD, H = m_TerrainH;
    std::vector<int> heightMap(W * D, 0);
    for (int z = 0; z < D; ++z)
    {
        for (int x = 0; x < W; ++x)
        {
            // 归一坐标 [0,1]
            double nx = (double)x / (double)W;
            double nz = (double)z / (double)D;

            // 小幅 domain warp 去除网格感
            double wx = nx, wz = nz;
            perlin.domainWarp2D(wx, wz, (double)m_Noise.warpScale, (double)m_Noise.warpAmp);

            // 低频群落权重（0..1），调制山地比重
            double biome = perlin.fbm2D(nx * (double)m_Noise.biomeScale, nz * (double)m_Noise.biomeScale, 3, 2.0, 0.5);
            biome = std::clamp(biome * (double)m_Noise.mountainWeight, 0.0, 1.0);

            // 大洲层：决定“大陆-海洋”的宏观起伏
            double continent = perlin.fbm2D(nx * (double)m_Noise.continentScale,
                                            nz * (double)m_Noise.continentScale,
                                            4, 2.0, 0.5); // 0..1
            continent = (continent * 2.0 - 1.0); // [-1,1]
            continent = std::clamp(continent * (double)m_Noise.continentStrength, -1.0, 1.0);
            // 映射至 0..1，正值抬升，负值压低
            double continentLift = (continent + 1.0) * 0.5; // 0..1

            // 平原（柔和）与山地（脊状）两套噪声
            double plains = perlin.fbm2D(wx * (double)m_Noise.plainsScale, wz * (double)m_Noise.plainsScale,
                                         m_Noise.plainsOctaves, (double)m_Noise.plainsLacunarity, (double)m_Noise.plainsGain); // 0..1
            double mountain = perlin.ridged2D(wx * (double)m_Noise.mountainScale, wz * (double)m_Noise.mountainScale,
                                              m_Noise.mountainOctaves, (double)m_Noise.mountainLacunarity, (double)m_Noise.mountainGain); // 0..1
            // 山脊锐度提升
            mountain = std::pow(std::clamp(mountain, 0.0, 1.0), 1.0 / std::max(0.1, (double)m_Noise.ridgeSharpness));

            // 按群落混合
            // 细节层（打花纹 & 小起伏）
            double detail = perlin.fbm2D(wx * (double)m_Noise.detailScale, wz * (double)m_Noise.detailScale,
                                         3, 2.0, 0.5);
            // 核心高度合成
            double h01 = plains * (1.0 - biome) + mountain * biome; // 0..1
            h01 = h01 + (detail - 0.5) * 2.0 * (double)m_Noise.detailAmp; // [-detailAmp, +detailAmp]
            h01 = std::clamp(h01, 0.0, 1.0);
            // 注入大陆大尺度抬升/压低
            h01 = (h01 * 0.8 + continentLift * 0.2);

            // 台地整形（可选）：将高度拉向 1/steps 的分段
            if (m_Noise.plateau > 0.0f)
            {
                const double steps = std::max(1, m_Noise.stepLevels);
                double stepped = std::floor(h01 * steps) / steps;
                h01 = h01 * (1.0 - (double)m_Noise.plateau) + stepped * (double)m_Noise.plateau;
            }

            // 高度分布调整：指数曲线（>1 提升高区占比）
            h01 = std::pow(std::clamp(h01, 0.0, 1.0), 1.0 / std::max(0.1, (double)m_Noise.curveExponent));

            // 谷地下切：加强低处沟壑（适度）
            if (m_Noise.valleyDepth > 0.0f)
            {
                double v = (0.5 - h01);
                h01 -= v * (double)m_Noise.valleyDepth; // 低处更低，高处几乎不变
                h01 = std::clamp(h01, 0.0, 1.0);
            }

            // 海平面抬升
            h01 = std::max(h01, (double)m_Noise.seaLevel);

            // 映射到体素高度
            double maxH = (double)(H - 1);
            int h = (int)glm::round(h01 * (double)m_Noise.heightMul * maxH);
            h = glm::clamp(h, 0, H - 1);
            heightMap[z * W + x] = h;
        }
    }

    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(W * D * H * 6 * 11 / 2);
    indices.reserve(W * D * H * 6);

    auto pushFace = [&](const glm::vec3 v[4], const std::array<glm::vec2, 4> &uv, uint32_t &baseIdx)
    {
        const float rC = 1.0f, gC = 1.0f, bC = 1.0f, aC = 1.0f, texIndex = 0.0f, tiling = 1.0f;
        for (int i = 0; i < 4; ++i)
        {
            vertices.push_back(v[i].x);
            vertices.push_back(v[i].y);
            vertices.push_back(v[i].z);
            vertices.push_back(rC);
            vertices.push_back(gC);
            vertices.push_back(bC);
            vertices.push_back(aC);
            vertices.push_back(uv[i].x);
            vertices.push_back(uv[i].y);
            vertices.push_back(texIndex);
            vertices.push_back(tiling);
        }
        indices.push_back(baseIdx + 0);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 3);
        indices.push_back(baseIdx + 0);
        baseIdx += 4;
    };

    auto quadUV = [](const glm::vec4 &r)
    {
        return std::array<glm::vec2, 4>{glm::vec2{r.x, r.y}, glm::vec2{r.z, r.y}, glm::vec2{r.z, r.w},
                                        glm::vec2{r.x, r.w}};
    };

    auto uvFor = [&](BlockType t, int face) -> std::array<glm::vec2, 4>
    {
        AtlasTile tile{0, 0};
        switch (t)
        {
            case GRASS:
                tile = (face == 4 ? m_GrassUV.top : face == 5 ? m_GrassUV.bottom : m_GrassUV.side);
                break;
            case DIRT:
                tile = m_DirtUV.side;
                break;
            case STONE:
                tile = m_StoneUV.side;
                break;
            default:
                break;
        }
        glm::vec4 r = AtlasUVRect(tile.col, tile.row, m_AtlasCols, m_AtlasRows, m_GridPadding);
        return quadUV(r);
    };

    uint32_t baseIdx = 0;
    const float s = 0.5f;
    for (int z = 0; z < D; ++z)
        for (int x = 0; x < W; ++x)
        {
            int h = heightMap[z * W + x];
            for (int y = 0; y <= h; ++y)
            {
                BlockType bt = (y == h) ? GRASS : (y >= h - 3 ? DIRT : STONE);

                bool posX = (x + 1 >= W) || (heightMap[z * W + (x + 1)] < y);
                bool negX = (x - 1 < 0) || (heightMap[z * W + (x - 1)] < y);
                bool posZ = (z + 1 >= D) || (heightMap[(z + 1) * W + x] < y);
                bool negZ = (z - 1 < 0) || (heightMap[(z - 1) * W + x] < y);
                bool top = (y + 1 > h);
                bool down = (y == 0);

                float cx = (float)x, cy = (float)y, cz = (float)z;
                glm::vec3 vF[4] = {{cx - s, cy - s, cz + s},
                                   {cx + s, cy - s, cz + s},
                                   {cx + s, cy + s, cz + s},
                                   {cx - s, cy + s, cz + s}};
                glm::vec3 vB[4] = {{cx + s, cy - s, cz - s},
                                   {cx - s, cy - s, cz - s},
                                   {cx - s, cy + s, cz - s},
                                   {cx + s, cy + s, cz - s}};
                glm::vec3 vL[4] = {{cx - s, cy - s, cz - s},
                                   {cx - s, cy - s, cz + s},
                                   {cx - s, cy + s, cz + s},
                                   {cx - s, cy + s, cz - s}};
                glm::vec3 vR[4] = {{cx + s, cy - s, cz + s},
                                   {cx + s, cy - s, cz - s},
                                   {cx + s, cy + s, cz - s},
                                   {cx + s, cy + s, cz + s}};
                glm::vec3 vT[4] = {{cx - s, cy + s, cz + s},
                                   {cx + s, cy + s, cz + s},
                                   {cx + s, cy + s, cz - s},
                                   {cx - s, cy + s, cz - s}};
                glm::vec3 vD_[4] = {{cx - s, cy - s, cz - s},
                                    {cx + s, cy - s, cz - s},
                                    {cx + s, cy - s, cz + s},
                                    {cx - s, cy - s, cz + s}};

                if (posZ)
                    pushFace(vF, uvFor(bt, 0), baseIdx);
                if (negZ)
                    pushFace(vB, uvFor(bt, 1), baseIdx);
                if (negX)
                    pushFace(vL, uvFor(bt, 2), baseIdx);
                if (posX)
                    pushFace(vR, uvFor(bt, 3), baseIdx);
                if (top)
                    pushFace(vT, uvFor(bt, 4), baseIdx);
                if (down)
                    pushFace(vD_, uvFor(bt, 5), baseIdx);
            }
        }

    if (vertices.empty() || indices.empty())
        return;

    auto vb = Himii::VertexBuffer::Create(vertices.data(), (uint32_t)(vertices.size() * sizeof(float)));
    Himii::BufferLayout layout = {
            {Himii::ShaderDataType::Float3, "a_Position"},    {Himii::ShaderDataType::Float4, "a_Color"},
            {Himii::ShaderDataType::Float2, "a_TexCoord"},    {Himii::ShaderDataType::Float, "a_TexIndex"},
            {Himii::ShaderDataType::Float, "a_TilingFactor"},
    };
    vb->SetLayout(layout);
    m_TerrainVA->AddVertexBuffer(vb);

    auto ib = Himii::IndexBuffer::Create(indices.data(), static_cast<uint32_t>(indices.size()));
    m_TerrainVA->SetIndexBuffer(ib);
}
