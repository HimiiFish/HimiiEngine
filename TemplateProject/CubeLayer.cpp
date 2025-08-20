#include "CubeLayer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glad/glad.h"
#include "Himii/Renderer/RenderCommand.h"
#include "imgui.h"
#include <array>

CubeLayer::CubeLayer()
    : Himii::Layer("CubeLayer")
    , m_CameraController(1280.0f / 720.0f) // 直接复用现有正交相机控制器
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
    return { u0, v0, u1, v1 };
}

glm::vec4 CubeLayer::AtlasUVRectPixels(int x, int y, int w, int h, int atlasW, int atlasH, bool originTopLeft)
{
    if (atlasW <= 0 || atlasH <= 0 || w <= 0 || h <= 0)
        return { 0.0f, 0.0f, 0.0f, 0.0f };

    // 若原点在左上，需要转换到 OpenGL 使用的左下原点
    int yBottom = originTopLeft ? (atlasH - y - h) : y;

    float u0 = static_cast<float>(x) / static_cast<float>(atlasW);
    float v0 = static_cast<float>(yBottom) / static_cast<float>(atlasH);
    float u1 = static_cast<float>(x + w) / static_cast<float>(atlasW);
    float v1 = static_cast<float>(yBottom + h) / static_cast<float>(atlasH);
    return { u0, v0, u1, v1 };
}

void CubeLayer::OnAttach()
{
    // 开启深度测试（若已在 RendererAPI 初始化中开启，可去掉）
    //glEnable(GL_DEPTH_TEST);

    // 1) 顶点数据：6个面(前后左右上下)，每面4个顶点：pos(x,y,z) + uv(u,v)
    m_CubeVA = Himii::VertexArray::Create();

    // 顶点结构匹配 TemplateProject/assets/shaders/Texture.glsl
    // layout(location=0) a_Position (vec3)
    // layout(location=1) a_Color    (vec4)
    // layout(location=2) a_TexCoord (vec2)
    // layout(location=3) a_TexIndex (float)
    // layout(location=4) a_TilingFactor (float)
    const uint32_t kVertexStrideFloats = 3 + 4 + 2 + 1 + 1; // 11
    auto vb = Himii::VertexBuffer::Create(24u * kVertexStrideFloats * sizeof(float));
    // 先放空数据，后面根据 atlas 设置/更新
    Himii::BufferLayout layout = {
        { Himii::ShaderDataType::Float3, "a_Position" },
        { Himii::ShaderDataType::Float4, "a_Color" },
        { Himii::ShaderDataType::Float2, "a_TexCoord" },
        { Himii::ShaderDataType::Float,  "a_TexIndex" },
        { Himii::ShaderDataType::Float,  "a_TilingFactor" },
    };
    vb->SetLayout(layout);
    m_CubeVA->AddVertexBuffer(vb);

    // 2) 索引：每面两个三角形，6个索引，共 6 面 = 36
    uint32_t indices[36];
    {
        uint32_t idx = 0;
        for (int face = 0; face < 6; ++face)
        {
            uint32_t base = face * 4;
            indices[idx++] = base + 0;
            indices[idx++] = base + 1;
            indices[idx++] = base + 2;
            indices[idx++] = base + 2;
            indices[idx++] = base + 3;
            indices[idx++] = base + 0;
        }
    }
    auto ib = Himii::IndexBuffer::Create(indices, 36);
    m_CubeVA->SetIndexBuffer(ib);

    // 3) 载入贴图、着色器（先用现有纹理，后续可换成图集）
    m_Atlas = Himii::Texture2D::Create("assets/textures/blocks.png");
    // 自动设置像素尺寸，便于 Pixel 模式直接使用
    if (m_Atlas)
    {
        m_AtlasWidth = (int)m_Atlas->GetWidth();
        m_AtlasHeight = (int)m_Atlas->GetHeight();
    }
    m_TextureShader = m_ShaderLibrary.Load("assets/shaders/Texture.glsl");
    m_TextureShader->Bind();
    // 设置采样器数组 [0..31]，确保 v_TexIndex=0 时能采样到纹理槽0
    int samplers[32];
    for (int i = 0; i < 32; ++i) samplers[i] = i;
    m_TextureShader->SetIntArray("u_Texture", samplers, 32);

    // 4) 构建一次顶点：单位立方体 [-0.5,0.5]
    RebuildVB();
}

void CubeLayer::RebuildVB()
{
    // 6个面的 uv 矩形
    glm::vec4 uvFront, uvBack, uvLeft, uvRight, uvTop, uvBottom;
    if (m_Mode == AtlasMode::Grid)
    {
        uvFront  = AtlasUVRect(m_Front.col,  m_Front.row,  m_AtlasCols, m_AtlasRows, m_GridPadding);
        uvBack   = AtlasUVRect(m_Back.col,   m_Back.row,   m_AtlasCols, m_AtlasRows, m_GridPadding);
        uvLeft   = AtlasUVRect(m_Left.col,   m_Left.row,   m_AtlasCols, m_AtlasRows, m_GridPadding);
        uvRight  = AtlasUVRect(m_Right.col,  m_Right.row,  m_AtlasCols, m_AtlasRows, m_GridPadding);
        uvTop    = AtlasUVRect(m_Top.col,    m_Top.row,    m_AtlasCols, m_AtlasRows, m_GridPadding);
        uvBottom = AtlasUVRect(m_Bottom.col, m_Bottom.row, m_AtlasCols, m_AtlasRows, m_GridPadding);
    }
    else
    {
        uvFront  = AtlasUVRectPixels(m_FrontPx.x,  m_FrontPx.y,  m_FrontPx.w,  m_FrontPx.h,  m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
        uvBack   = AtlasUVRectPixels(m_BackPx.x,   m_BackPx.y,   m_BackPx.w,   m_BackPx.h,   m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
        uvLeft   = AtlasUVRectPixels(m_LeftPx.x,   m_LeftPx.y,   m_LeftPx.w,   m_LeftPx.h,   m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
        uvRight  = AtlasUVRectPixels(m_RightPx.x,  m_RightPx.y,  m_RightPx.w,  m_RightPx.h,  m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
        uvTop    = AtlasUVRectPixels(m_TopPx.x,    m_TopPx.y,    m_TopPx.w,    m_TopPx.h,    m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
        uvBottom = AtlasUVRectPixels(m_BottomPx.x, m_BottomPx.y, m_BottomPx.w, m_BottomPx.h, m_AtlasWidth, m_AtlasHeight, m_OriginTopLeft);
    }

    auto quadUV = [](const glm::vec4& r) -> std::array<glm::vec2,4> {
        // (u0,v0)=左下, (u1,v1)=右上
        return { glm::vec2{r.x, r.y}, glm::vec2{r.z, r.y}, glm::vec2{r.z, r.w}, glm::vec2{r.x, r.w} };
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
    glm::vec3 f[4] = { {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s} };
    glm::vec3 b[4] = { { s,-s,-s},{-s,-s,-s},{-s, s,-s},{ s, s,-s} };
    glm::vec3 l[4] = { {-s,-s,-s},{-s,-s, s},{-s, s, s},{-s, s,-s} };
    glm::vec3 r[4] = { { s,-s, s},{ s,-s,-s},{ s, s,-s},{ s, s, s} };
    glm::vec3 t[4] = { {-s, s, s},{ s, s, s},{ s, s,-s},{-s, s,-s} };
    glm::vec3 d[4] = { {-s,-s,-s},{ s,-s,-s},{ s,-s, s},{-s,-s, s} };

    auto pushFace = [&](glm::vec3 v[4], std::array<glm::vec2,4>& uvs) {
        const float rC = 1.0f, gC = 1.0f, bC = 1.0f, aC = 1.0f;
        const float texIndex = 0.0f;
        const float tiling = 1.0f;
        for (int i = 0; i < 4; ++i) {
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
    glDisable(GL_DEPTH_TEST);
}

void CubeLayer::OnUpdate(Himii::Timestep ts)
{
    m_CameraController.OnUpdate(ts);

    // 渲染准备：清颜色与深度缓冲
    Himii::RenderCommand::SetClearColor({0.1f, 0.12f, 0.16f, 1.0f});
    Himii::RenderCommand::Clear();

    // 旋转
    m_Angle += m_RotateSpeed * (float)ts;
    glm::mat4 transform(1.0f);
    transform = glm::rotate(transform, glm::radians(m_Angle), glm::vec3(0.3f, 1.0f, 0.2f));

    // 提交绘制
    Himii::Renderer::BeginScene(m_CameraController.GetCamera());
    m_Atlas->Bind(0);
    Himii::Renderer::Submit(m_TextureShader, m_CubeVA, transform);
    Himii::Renderer::EndScene();
}

void CubeLayer::OnImGuiRender()
{
    ImGui::Begin("Cube Settings");
    ImGui::Text("均值延迟 %.3f ms/帧率 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::DragFloat("Rotate Speed (deg/s)", &m_RotateSpeed, 1.0f, -360.0f, 360.0f);

    bool rebuild = false;
    int mode = (m_Mode == AtlasMode::Grid) ? 0 : 1;
    if (ImGui::Combo("Atlas Mode", &mode, "Grid\0Pixel\0")) { m_Mode = (mode==0?AtlasMode::Grid:AtlasMode::Pixel); rebuild = true; }

    if (m_Mode == AtlasMode::Grid)
    {
        rebuild |= ImGui::DragInt("Atlas Cols", &m_AtlasCols, 1, 1, 64);
        rebuild |= ImGui::DragInt("Atlas Rows", &m_AtlasRows, 1, 1, 64);
        rebuild |= ImGui::DragFloat("Grid Padding", &m_GridPadding, 0.0005f, 0.0f, 0.05f);
        ImGui::SeparatorText("Face Tiles (col,row)");
        rebuild |= ImGui::DragInt2("Front",  &m_Front.col);
        rebuild |= ImGui::DragInt2("Back",   &m_Back.col);
        rebuild |= ImGui::DragInt2("Left",   &m_Left.col);
        rebuild |= ImGui::DragInt2("Right",  &m_Right.col);
        rebuild |= ImGui::DragInt2("Top",    &m_Top.col);
        rebuild |= ImGui::DragInt2("Bottom", &m_Bottom.col);
    }
    else
    {
        ImGui::SeparatorText("Atlas Size (px)");
        rebuild |= ImGui::DragInt("Atlas W", &m_AtlasWidth, 1, 1, 8192);
        rebuild |= ImGui::DragInt("Atlas H", &m_AtlasHeight, 1, 1, 8192);
        rebuild |= ImGui::Checkbox("Origin Top-Left", &m_OriginTopLeft);

        auto rectEdit = [&](const char* name, AtlasRect& r){ return ImGui::DragInt4(name, &r.x); };
        ImGui::SeparatorText("Face Rect (x,y,w,h)");
        rebuild |= rectEdit("FrontPx",  m_FrontPx);
        rebuild |= rectEdit("BackPx",   m_BackPx);
        rebuild |= rectEdit("LeftPx",   m_LeftPx);
        rebuild |= rectEdit("RightPx",  m_RightPx);
        rebuild |= rectEdit("TopPx",    m_TopPx);
        rebuild |= rectEdit("BottomPx", m_BottomPx);
    }

    if (rebuild) RebuildVB();
    ImGui::End();
}

void CubeLayer::OnEvent(Himii::Event& e)
{
    m_CameraController.OnEvent(e);
}